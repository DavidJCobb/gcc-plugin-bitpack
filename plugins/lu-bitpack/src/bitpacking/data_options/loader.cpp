#include "bitpacking/data_options/loader.h"
#include <stdexcept>
#include <type_traits>
#include "bitpacking/data_options/x_options.h"
#include "bitpacking/heritable_options.h"
#include "bitpacking/transform_function_validation_helpers.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/type/array.h"
#include <string_view>
#include <stringpool.h> // get_identifier
#include <c-family/c-common.h> // lookup_name
#include <diagnostic.h> // warning(), error(), etc.
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
   
   gw::decl::function _get_func_decl(const char* identifier) {
      auto node = lookup_name(get_identifier(identifier));
      if (node != NULL_TREE && !gw::decl::function::node_is(node))
         node = NULL_TREE;
      return gw::decl::function::from_untyped(node);
   }
}

namespace {
   std::string _name_of(tree node) {
      assert(node != NULL_TREE);
      std::string result;
      if (gw::type::base::node_is(node)) {
         result = "type %<";
         result += gw::type::base::from_untyped(node).pretty_print();
         result += "%>";
         return result;
      }
      if (gw::decl::field::node_is(node)) {
         auto decl = gw::decl::field::from_untyped(node);
         
         std::string result = "field %<";
         
         auto context = decl.context();
         if (TYPE_P(context.as_untyped())) {
            result += gw::type::base::from_untyped(context.as_untyped()).pretty_print();
            result += "::";
         }
         result += decl.name();
         result += "%>";
         return result;
      }
      return "<?>";
   }
   location_t _location_of(tree node) {
      gw::decl::base decl;
      if (gw::type::base::node_is(node)) {
         decl = gw::type::base::from_untyped(node).declaration();
      } else if (gw::decl::field::node_is(node)) {
         decl = gw::decl::field::from_untyped(node);
      }
      if (decl.empty())
         return UNKNOWN_LOCATION;
      return decl.source_location();
   }

   template<typename... Args>
   void _report_warning(tree subject, const char* fmt, Args&&... args) {
      std::string message = "unusual bitpacking options for ";
      message += _name_of(subject);
      message += ": ";
      message += fmt;
      warning_at(_location_of(subject), OPT_Wpragmas, message.c_str(), args...);
   }
   
   template<typename... Args>
   void _report_error(tree subject, const char* fmt, Args&&... args) {
      std::string message = "invalid bitpacking options for ";
      message += _name_of(subject);
      message += ": ";
      message += fmt;
      error_at(_location_of(subject), message.c_str(), args...);
   }
   
   static std::string_view _name_of_coalescing_type(const bitpacking::data_options::requested_x_options::variant& data) {
      if (std::holds_alternative<std::monostate>(data)) {
         return "<none>";
      }
      if (std::holds_alternative<bitpacking::data_options::requested_x_options::integral>(data)) {
         return "integer or integer-like";
      }
      if (std::holds_alternative<bitpacking::data_options::requested_x_options::string>(data)) {
         return "string";
      }
      return "???";
   }
}

namespace bitpacking::data_options {
   void loader::coalescing_options::try_coalesce(tree report_errors_on, const heritable_options& heritable) {
      const auto& src = heritable.data;
      if (std::holds_alternative<std::monostate>(src)) {
         return;
      }
      
      auto& dst = this->data;
      if (std::holds_alternative<std::monostate>(dst)) {
         dst = src;
         return;
      }
      if (dst.index() != src.index()) {
         _report_error(
            report_errors_on,
            "heritable option set %<%s%> is of the wrong type (their type: %s; this declaration is already using options of type: %s)",
            heritable.name.c_str(),
            _name_of_coalescing_type(src),
            _name_of_coalescing_type(dst)
         );
      } else {
         if (auto* casted = std::get_if<requested_x_options::integral>(&src)) {
            std::get<requested_x_options::integral>(dst).coalesce(*casted);
         } else if (auto* casted = std::get_if<requested_x_options::string>(&src)) {
            std::get<requested_x_options::string>(dst).coalesce(*casted);
         }
      }
   }
   
   void loader::_coalesce_heritable(tree report_errors_on, coalescing_options& dst, const heritable_options& heritable) {
      //
      // Apply transforms.
      //
      {
         const auto blame = std::string("inherited options %<") + heritable.name + "%> specify ";
         
         const auto& src_transform = heritable.transforms;
         
         auto _grab = [report_errors_on, heritable, &blame](gw::decl::function& dst, const std::string& identifier, bool is_pre_pack) {
            if (identifier.empty())
               return;
            auto decl = dst = _get_func_decl(identifier.c_str());
            if (decl.empty()) {
               _report_error(
                  report_errors_on,
                  "inherited options %<%s%> specified %s transform function %<%s%>; this identifier either does not exist or is not a function",
                  heritable.name.c_str(),
                  is_pre_pack ? "pre-pack" : "post-unpack",
                  identifier.c_str()
               );
            } else {
               bool valid;
               if (is_pre_pack)
                  valid = can_be_pre_pack_function(decl);
               else
                  valid = can_be_post_unpack_function(decl);
               
               if (!valid) {
                  _report_error(
                     report_errors_on,
                     "%<lu_bitpack_funcs%> attribute specifies a %s function with the wrong signature; signature should be <%void %s(%sNormalType*, %sPackedType*)%>.",
                     is_pre_pack ? "pre-pack" : "post-unpack",
                     decl.name().data(),
                     is_pre_pack ? "const " : "",
                     is_pre_pack ? "" : "const "
                  );
               }
            }
         };
         //
         _grab(dst.transforms.pre_pack,    src_transform.pre_pack   , true);
         _grab(dst.transforms.post_unpack, src_transform.post_unpack, false);
      }
      //
      // Apply other options.
      //
      dst.try_coalesce(report_errors_on, heritable);
   }
   void loader::_coalesce_attributes(tree report_errors_on, coalescing_options& dst, const requested_via_attributes& src) {
      if (src.omit)
         dst.omit = true;
      
      if (!src.transforms.pre_pack.empty())
         dst.transforms.pre_pack = src.transforms.pre_pack;
      if (!src.transforms.post_unpack.empty())
         dst.transforms.post_unpack = src.transforms.post_unpack;
      
      constexpr const auto type_mismatch_fmt = "options from this declaration's attributes are of the wrong type (their type: %s; this declaration is already using options of type: %s)";
      
      if (src.as_opaque_buffer) {
         if (!std::holds_alternative<std::monostate>(dst.data)) {
            _report_error(report_errors_on, type_mismatch_fmt, _name_of_coalescing_type(dst.data), "opaque buffer");
            return;
         }
         dst.as_opaque_buffer = true;
         return;
      }
      
      auto _do_coalesce = [report_errors_on, &dst](const auto& src) {
         if (dst.as_opaque_buffer) {
            _report_error(report_errors_on, type_mismatch_fmt, _name_of_coalescing_type(src), "opaque buffer");
            return;
         }
         if (std::holds_alternative<std::monostate>(dst.data)) {
            dst.data = src;
            return;
         }
         
         using src_type = std::decay_t<decltype(src)>;
         if (std::holds_alternative<src_type>(dst.data)) {
            std::get<src_type>(dst.data).coalesce(src);
            return;
         }
         _report_error(
            report_errors_on,
            type_mismatch_fmt,
            _name_of_coalescing_type(src),
            _name_of_coalescing_type(dst.data)
         );
      };
      
      if (auto& typed_src_opt = src.x_options.string; typed_src_opt.has_value()) {
         _do_coalesce(*typed_src_opt);
         return;
      }
      
      if (auto& typed_src = src.x_options.integral; !typed_src.empty()) {
         _do_coalesce(typed_src);
         return;
      }
   }
   
   loader::result_code loader::go() {
      assert(!this->field.empty());
      auto type = this->field.value_type();
      if (type.is_array()) {
         auto array_type = type.as_array();
         auto value_type = array_type.value_type();
         do {
            if (!value_type.is_array())
               break;
            array_type = value_type.as_array();
            value_type = array_type.value_type();
         } while (true);
         type = value_type;
      }
      
      struct {
         requested_via_attributes type;
         requested_via_attributes field;
      } seen_options;
      
      //
      // Extract what data we can.
      //
      if (!seen_options.type.load(type))
         return result_code::failed;
      if (!seen_options.field.load(this->field))
         return result_code::failed;
      
      //
      // Coalesce the options and check for contradictions.
      //
      auto errors_prior = errorcount; // diagnostic.h
      
      coalescing_options coalescing;
      
      if (auto* heritable = seen_options.type.inherit) {
         _coalesce_heritable(type.as_untyped(), coalescing, *heritable);
      }
      _coalesce_attributes(type.as_untyped(), coalescing, seen_options.type);
      
      if (auto* heritable = seen_options.field.inherit) {
         _coalesce_heritable(this->field.as_untyped(), coalescing, *heritable);
      }
      _coalesce_attributes(this->field.as_untyped(), coalescing, seen_options.field);
      
      //
      // Check that the transform functions match.
      //
      {
         auto a = coalescing.transforms.pre_pack;
         auto b = coalescing.transforms.post_unpack;
         if (a.empty() != b.empty()) {
            if (a.empty()) {
               _report_error(this->field.as_untyped(), "this data member specifies (or inherits) only a post-unpack transform function; you must specify both pre-pack and post-unpack functions, or neither");
            } else {
               _report_error(this->field.as_untyped(), "this data member specifies (or inherits) only a pre-pack transform function; you must specify both pre-pack and post-unpack functions, or neither");
            }
         } else if (!a.empty()) {
            //
            // Re-check this condition so that `check_transform_functions_match_types` 
            // doesn't have to, but don't emit an error message if it's not met because 
            // we already will have when loading the attributes and heritables.
            //
            if (can_be_transform_function(a) && can_be_transform_function(b)) {
               auto problem = check_transform_functions_match_types(
                  coalescing.transforms.pre_pack,
                  coalescing.transforms.post_unpack
               );
               if (!problem.empty()) {
                  problem = std::string("the transform functions that this data member has ended up using don't match: ") + problem;
                  _report_error(this->field.as_untyped(), problem.c_str());
               }
            }
         }
      }
      
      //
      // Apply the options.
      //
      
      if (coalescing.omit)
         return result_code::field_is_omitted;
      
      auto& dst_result = this->result.emplace();
      dst_result.transforms.pre_pack    = coalescing.transforms.pre_pack;
      dst_result.transforms.post_unpack = coalescing.transforms.post_unpack;
      
      if (coalescing.as_opaque_buffer) {
         dst_result.kind = member_kind::buffer;
         dst_result.data.emplace<computed_x_options::buffer>().bytecount = type.size_in_bytes();
      } else if (const auto* casted = std::get_if<requested_x_options::integral>(&coalescing.data)) {
         auto& src = *casted;
         auto& dst = dst_result.data.emplace<computed_x_options::integral>();
         try {
            dst = src.bake(global, this->field, type, dst_result.kind);
         } catch (std::runtime_error& ex) {
            _report_error(this->field.as_untyped(), ex.what());
         }
      } else if (const auto* casted = std::get_if<requested_x_options::string>(&coalescing.data)) {
         dst_result.kind = member_kind::string;
         auto& src = *casted;
         auto& dst = dst_result.data.emplace<computed_x_options::string>();
         try {
            dst = src.bake(global, this->field, type);
         } catch (std::runtime_error& ex) {
            _report_error(this->field.as_untyped(), ex.what());
         }
      } else {
         //
         // No type-specific options specified.
         //
         assert(!type.is_union() && "unions are not yet implemented!");
         if (type.is_record()) {
            dst_result.kind = member_kind::structure;
         } else if (type.is_boolean() || type.is_enum() || type.is_integer() || type.is_pointer()) {
            try {
               dst_result.data = requested_x_options::integral{}.bake(global, this->field, type, dst_result.kind);
            } catch (std::runtime_error& ex) {
               _report_error(this->field.as_untyped(), ex.what());
            }
         } else {
            assert(false && "no bitpacking options + unhandled member type");
         }
      }
      
      // Done!
      
      auto errors_after = errorcount;
      if (errors_prior < errors_after) {
         return result_code::failed;
      }
      return result_code::success;
   }
}