#include "bitpacking/data_options.h"
#include <bit>
#include <cassert>
#include <inttypes.h> // PRIdMAX and friends
#include <stdexcept>
#include "lu/strings/handle_kv_string.h"
#include "lu/strings/printf_string.h"
#include "bitpacking/global_options.h"
#include "bitpacking/heritable_options.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/builtin_types.h"
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

namespace bitpacking {
   std::string data_options_loader::_context_name() const {
      if (auto* casted = std::get_if<gw::type::base>(this->context.attributes_of)) {
         std::string result = "type ";
         result += casted->pretty_print();
         return result;
      }
      if (auto* casted = std::get_if<gw::decl::field>(this->context.attributes_of)) {
         std::string result = "field ";
         
         auto context = casted->context();
         if (TYPE_P(context.as_untyped())) {
            result += gw::type::base::from_untyped(context.as_untyped()).pretty_print();
            result += "::";
         }
         result += casted->name();
         return result;
      }
      return "<?>";
   }
   location_t data_options_loader::_context_location() const {
      gw::decl::base decl;
      if (auto* casted = std::get_if<gw::type::base>(this->context.attributes_of)) {
         decl = casted->declaration();
      } else if (auto* casted = std::get_if<gw::decl::field>(this->context.attributes_of)) {
         decl = *casted;
      }
      if (decl.empty())
         return UNKNOWN_LOCATION;
      return decl.source_location();
   }
   
   template<typename... Args>
   void data_options_loader::_report_warning(const char* fmt, Args&&...) const {
      std::string message = "unusual bitpacking options for %<";
      message += this->_context_name();
      message += "%>: ";
      message += fmt;
      warning_at(this->_context_location(), OPT_Wpragmas, message.c_str(), args...);
   }
   
   template<typename... Args>
   void data_options_loader::_report_error(const char* fmt, Args&&...) const {
      std::string message = "invalid bitpacking options for %<";
      message += this->_context_name();
      message += "%>: ";
      message += fmt;
      error_at(this->_context_location(), message.c_str(), args...);
      this->success = false;
   }
   
   void data_options_loader::_warn_on_extra_attribute_args(gcc_wrappers::attribute attr, size_t expected) const {
      auto argcount = attr.arguments().size();
      if (argcount <= expected)
         return;
      _report_warning(
         "%<%s%> attribute specifies more (%u) arguments than expected (%u)",
         attr.name().data(),
         (unsigned int)attr.arguments().size(),
         (unsigned int)expected
      );
   }
   
   std::string data_options_loader::_pull_heritable_name(gcc_wrappers::attribute_list) const {
      auto attr = this->context.attributes.get_attribute("lu_bitpack_inherit");
      if (attr.empty())
         return {};
      auto args = attr.arguments();
      if (args.has_leading_identifier()) {
         _report_error(
            "%<%s%> attribute must not have an identifier as its first argument.",
            attr.name().data()
         );
         return {};
      }
      if (args.empty()) {
         _report_error("%<lu_bitpack_inherit%> attribute must have a string constant argument identifying the set of heritable options to use");
         return {};
      }
      _warn_on_extra_attribute_args(attr, 1);
      
      auto arg = args.front().as<gw::expr::string_constant>();
      if (arg.empty()) {
         _report_error("%<lu_bitpack_inherit%> attribute must have a string constant argument identifying the set of heritable options to use");
         return {};
      }
      auto name = arg.value();
      if (name.empty()) {
         _report_error("%<lu_bitpack_inherit%> heritable options name cannot be blank");
      }
      return name;
   }
         
   bool data_options_loader::_can_be_transform_function(gcc_wrappers::type::function type) const {
      auto args = type.arguments();
      if (args.size() != 3)
         return false;
      if (TREE_CODE(args.untyped_back()) != VOID_TYPE)
         return false;
      
      auto arg_a = type.nth_argument_type(0);
      auto arg_b = type.nth_argument_type(1);
      if (!arg_a.is_pointer() || !arg_b.is_pointer())
         return false;
      
      return true;
   }
   bool data_options_loader::_check_pre_pack_function(
      std::string_view blame,
      gw::decl::function decl,
      gw::type::function type
   ) const {
      if (
         !_check_transform_function(type) ||
         !type.nth_argument_type(1).remove_pointer().is_const()
      ) {
         this->_report_error("%<lu_bitpack_funcs%> attribute specifies a pre-pack function with the wrong signature; signature should be <%void %s(const NormalType*, PackedType*)%>.", decl.name().data());
      }
      return true;
   }
   bool data_options_loader::_check_post_unpack_function(
      std::string_view blame,
      gw::decl::function decl,
      gw::type::function type
   ) const {
      if (
         !_check_transform_function(type) ||
         !type.nth_argument_type(1).remove_pointer().is_const()
      ) {
         this->_report_error("%<lu_bitpack_funcs%> attribute specifies a post-unpack function with the wrong signature; signature should be <%void %s(NormalType*, const PackedType*)%>.", decl.name().data());
      }
      return true;
   }
   
   void data_options_loader::_check_transform_functions_match(gcc_wrappers::type::function pre_pack, gcc_wrappers::type::function post_unpack) {
      auto normal_type_a = pre_pack.nth_argument_type(0).remove_pointer().remove_const();
      auto packed_type_a = pre_pack.nth_argument_type(1).remove_pointer().remove_const();
      
      auto normal_type_b = post_unpack.nth_argument_type(0).remove_pointer().remove_const();
      auto packed_type_b = post_unpack.nth_argument_type(1).remove_pointer().remove_const();
      
      if (normal_type_a != normal_type_b) {
         auto as = normal_type_a.pretty_print();
         auto bs = normal_type_b.pretty_print();
         this->_report_error("%<lu_bitpack_funcs%> attribute specifies pre-pack and post-unpack functions whose normal-type argument doesn't match (%<%s%> versus %<%s%>)", as.c_str(), bs.c_str());
      }
      if (packed_type_a != packed_type_b) {
         auto as = packed_type_a.pretty_print();
         auto bs = packed_type_b.pretty_print();
         this->_report_error("%<lu_bitpack_funcs%> attribute specifies pre-pack and post-unpack functions whose packed-type argument doesn't match (%<%s%> versus %<%s%>)", as.c_str(), bs.c_str());
      }
   }
   
   void data_options_loader::_pull_option_set(option_set& dst) {
      const auto& ty = gw::builtin_types::get_fast();
      
      auto attributes     = this->context.attributes;
      auto heritable_name = _pull_heritable_name(attributes);
      if (!heritable_name.empty()) {
         auto* o = heritable_options_stockpile::get().options_by_name(heritable_name);
         if (!o) {
            _report_error("%<lu_bitpack_inherit%>: no heritable option set exists with the name %<%s%>", heritable_name.c_str());
         }
         dst.inherit = o;
      }
      
      for(auto attr : attributes) {
         auto key  = attr.name();
         auto args = attr.arguments();
         if (key == "lu_bitpack_inherit") {
            //
            // Already handled.
            //
            continue;
         }
         
         if (key == "lu_bitpack_omit") {
            dst.omit = true;
            continue;
         }
         if (key == "lu_bitpack_as_opaque_buffer") {
            dst.as_opaque_buffer = true;
            continue;
         }
         if (key == "lu_bitpack_bitcount") {
            if (args.empty()) {
               _report_error(
                  "%<lu_bitpack_bitcount%> attribute must have an integer constant parameter "
                  "indicating the number of bits to pack the field into."
               );
               continue;
            }
            _warn_on_extra_attribute_args(attr, 1);
            int v;
            {
               auto arg = args.front().as<gw::expr::integer_constant>();
               auto opt = arg.try_value_signed();
               if (!opt.has_value()) {
                  _report_error(
                     "%<lu_bitpack_bitcount%> attribute must have an integer constant parameter "
                     "indicating the number of bits to pack the field into."
                  );
                  continue;
               }
               v = (int) *opt;
            }
            if (v <= 0) {
               _report_error(
                  "%<lu_bitpack_bitcount%> attribute must specify a positive non-zero "
                  "bitcount (seen: %d)",
                  (int)v
               );
               continue;
            }
            if (v > (int)sizeof(size_t) * 8) {
               _report_warning(
                  "%<lu_bitpack_bitcount%> attribute specifies an unusually large bitcount; is this intentional? (seen: %<%d%>)",
                  (int)v
               );
            }
            dst.integral.bitcount = v;
            continue;
         }
         if (key == "lu_bitpack_funcs") {
            std::string id_pre_pack;
            std::string id_post_unpack;
            
            if (!args.empty()) {
               _warn_on_extra_attribute_args(attr, 1);
               auto arg = args.front().as<gw::expr::string_constant>();
               if (arg.empty()) {
                  _report_error(
                     "%<lu_bitpack_funcs%> attribute argument must be a string literal of "
                     "the form \"pre_pack=identifier,post_unpack=identifier\""
                  );
                  continue;
               }
               auto data = arg.value();
               try {
                  lu::strings::handle_kv_string(data, std::array{
                     lu::strings::kv_string_param{
                        .name      = "pre_pack",
                        .has_param = true,
                        .handler   = [&pre_pack](std::string_view v, int vi) {
                           id_pre_pack = v;
                        },
                     },
                     lu::strings::kv_string_param{
                        .name      = "post_unpack",
                        .has_param = false,
                        .handler   = [&post_unpack](std::string_view v, int vi) {
                           id_post_unpack = v;
                        },
                     },
                  });
               } catch (std::runtime_error& ex) {
                  _report_error(
                     "%<lu_bitpack_funcs%> attribute failed to parse: %s",
                     ex.what()
                  );
                  continue;
               }
            }
            
            if (id_pre_pack.empty() != id_post_unpack.empty()) {
               if (id_pre_pack.empty()) {
                  _report_error("%<lu_bitpack_funcs%> attribute specifies only a post-unpack function; you must specify both pre-pack and post-unpack functions");
               } else {
                  _report_error("%<lu_bitpack_funcs%> attribute specifies only a pre-pack function; you must specify both pre-pack and post-unpack functions");
               }
               continue;
            }
            
            dst.transforms.pre_pack    = _get_func_decl(id_pre_pack);
            dst.transforms.post_unpack = _get_func_decl(id_post_unpack);
            if (dst.transforms.pre_pack.empty()) {
               _report_error(
                  "%<lu_bitpack_funcs%> attribute specifies a pre-pack identifier %<%s%> that "
                  "either doesn't or is not a function",
                  id_pre_pack.c_str()
               );
            }
            if (dst.transforms.post_unpack.empty()) {
               _report_error(
                  "%<lu_bitpack_funcs%> attribute specifies a post-unpack identifier %<%s%> that "
                  "either doesn't or is not a function",
                  id_post_unpack.c_str()
               );
            }
            if (dst.transforms.pre_pack.empty() || dst.transforms.post_unpack.empty()) {
               continue;
            }
            
            auto decl_pre  = dst.transforms.pre_pack;
            auto decl_post = dst.transforms.post_unpack;
            auto type_pre  = decl_pre.function_type();
            auto type_post = decl_post.function_type();
            
            constexpr const std::string_view blame = "%<lu_bitpack_funcs%> attribute specifies";
            
            bool signatures_correct_ish = true;
            if (!_check_pre_pack_function(blame, type_pre, decl_pre)) {
               signatures_correct_ish = false;
            }
            if (!_check_post_unpack_function(blame, type_post, decl_post)) {
               signatures_correct_ish = false;
            }
            if (!signatures_correct_ish) {
               continue;
            }
            
            // TODO: Move this check to after we've resolved heritables, etc.
            _check_transform_functions_match(type_pre, type_post);
            continue;
         }
         if (key == "lu_bitpack_range") {
            if (args.size() < 2) {
               if (args.empty()) {
                  _report_error("%<lu_bitpack_range%> attribute must specify two arguments (the minimum and maximum possible values of the field); no arguments given");
               } else {
                  _report_error("%<lu_bitpack_range%> attribute must specify two arguments (the minimum and maximum possible values of the field); only one argument given");
               }
               continue;
            }
            _warn_on_extra_attribute_args(attr, 2);
            
            auto opt_a = args[0].as<gw::expr::integer_constant>().try_value_signed();
            auto opt_b = args[1].as<gw::expr::integer_constant>().try_value_signed();
            if (!opt_a.has_value() || !opt_b.has_value()) {
               if (!opt_a.has_value()) {
                  _report_error("%<lu_bitpack_range%> first argument doesn't appear to be an integer constant");
               }
               if (!opt_b.has_value()) {
                  _report_error("%<lu_bitpack_range%> second argument doesn't appear to be an integer constant");
               }
               continue;
            }
            dst.integral.min = *opt_a;
            dst.integral.max = *opt_b;
            continue;
         }
         if (key == "lu_bitpack_string") {
            auto type = decl.value_type();
            if (!type.is_array()) {
               _report_error(
                  "%<lu_bitpack_string%> used on a field that isn't a string (array of char)"
               );
               continue;
            }
            auto& dst_data = dst.string.has_value() ? *dst.string : dst.string.emplace();
            if (args.empty())
               continue;
            _warn_on_extra_attribute_args(attr, 1);
            
            auto arg = args.front().as<gw::expr::string_constant>();
            if (arg.empty()) {
               _report_error("%<lu_bitpack_string%> attribute argument must be a string literal");
               continue;
            }
            
            std::optional<int>  length;
            std::optional<bool> null_terminated = false;
            
            auto data = arg.value();
            try {
               lu::strings::handle_kv_string(data, std::array{
                  lu::strings::kv_string_param{
                     .name      = "length",
                     .has_param = true,
                     .int_param = true,
                     .handler   = [&length](std::string_view v, int vi) {
                        length = vi;
                     },
                  },
                  lu::strings::kv_string_param{
                     .name      = "with-terminator",
                     .has_param = false,
                     .handler   = [&null_terminated](std::string_view v, int vi) {
                        null_terminated = true;
                     },
                  },
               });
            } catch (std::runtime_error& ex) {
               _report_error(
                  "%<lu_bitpack_string%> attribute failed to parse: %s",
                  ex.what()
               );
               continue;
            }
            
            if (length.has_value()) {
               auto v = *length;
               if (v <= 0) {
                  _report_error(
                     "%<lu_bitpack_string%> attribute must specify a positive non-zero "
                     "string length (seen: %d)",
                     (int)v
                  );
                  continue;
               }
               dst_data.length = v;
            }
            if (null_terminated.has_value())
               dst_data.with_terminator = *null_terminated;
            continue;
         }
         
         if (key.starts_with("lu_bitpack_")) {
            _report_warning("unrecognized bitpacking attribute %<%s%>", key.data());
         }
      }
   }
   
   std::string_view data_options_loader::_name_of_coalescing_type(const in_progress_options::variant& data) const {
      if (std::holds_alternative<std::monostate>(data)) {
         return "<none>";
      }
      if (std::holds_alternative<in_progress_options::integral>(data)) {
         return "integer or integer-like";
      }
      if (std::holds_alternative<in_progress_options::string>(data)) {
         return "string";
      }
      return "???";
   }
   
   void data_options_loader::coalescing_options::try_coalesce(const heritable_options& heritable) {
      const auto& src = heritable.data;
      if (std::holds_alternative<std::monostate>(src)) {
         return;
      }
      
      auto& dst = dst.data;
      if (std::holds_alternative<std::monostate>(dst)) {
         dst = src;
         return;
      }
      if (dst.index() != src.index()) {
         _report_error(
            "inherited options %<%s%> are of the wrong type (inherited is type: %s; data member is already using options of type: %s)",
            heritable.name.c_str(),
            _name_of_coalescing_type(src),
            _name_of_coalescing_type(dst)
         );
      } else {
         if (auto* casted = std::get_if<in_progress_options::integral>(&src)) {
            std::get<in_progress_options::integral>(dst).coalesce(*casted);
         } else if (auto* casted = std::get_if<in_progress_options::string>(&src)) {
            std::get<in_progress_options::string>(dst).coalesce(*casted);
         }
      }
   }
   
   void data_options_loader::_coalesce_heritable(coalescing_options& dst, const heritable_options& heritable) {
      //
      // Apply transforms.
      //
      {
         const auto blame = std::string("inherited options %<") + heritable.name + "%> specify ";
         const auto what  = std::string_view("pre-pack");
         
         const auto& src_transform = heritable.transforms;
         bool pre_pack_okay    = false;
         bool post_unpack_okay = false;
         
         auto _grab = [heritable](gw::decl::function& dst, const char* what, const std::string& name) {
            if (name.empty())
               return;
            auto decl = this->result.transforms.pre_pack = _get_func_decl(name);
            if (decl.empty()) {
               _report_error(
                  "inherited options %<%s%> specified %s transform function %<%s%>; this identifier either does not exist or is not a function",
                  heritable.name.c_str(),
                  what,
                  name.c_str()
               );
            } else {
               auto fn_type = decl.function_type();
               pre_pack_okay = _check_pre_pack_function(blame, decl, fn_type);
            }
            dst = decl;
         };
         //
         _grab(this->result.transforms.pre_pack, "pre-pack", src_transform.pre_pack);
         _grab(this->result.transforms.post_unpack, "post-unpack", src_transform.post_unpack);
      }
      //
      // Apply other options.
      //
      dst.try_coalesce(heritable);
   }
   void data_options_loader::_coalesce_attributes(coalescing_options& dst, const option_set& src) {
      if (src.omit)
         dst.omit = true;
      
      constexpr const auto type_mismatch_fmt = "mismatch between inherited and local attributes (inherited: %s; local: %s)";
      
      if (src.as_opaque_buffer) {
         if (!std::holds_alternative<std::monostate>(dst.data)) {
            _report_error(type_mismatch_fmt, _name_of_coalescing_type(dst), "opaque buffer");
            return;
         }
         dst.as_opaque_buffer = true;
         return;
      }
      
      if (src.string.has_value()) {
         if (dst.as_opaque_buffer) {
            _report_error(type_mismatch_fmt, "opaque buffer", "string");
            return;
         }
         dst.data.emplace<in_progress_options::string>() = *src.string;
         return;
      }
      
      if (!src.integral.empty()) {
         if (dst.as_opaque_buffer) {
            _report_error(type_mismatch_fmt, "opaque buffer", "integer or integer-like");
            return;
         }
         dst.data.emplace<in_progress_options::integral>() = src.integral;
      }
   }
   
   void data_options_loader::go() {
      assert(!this->field.empty());
      auto type = this->field.value_type();
      
      //
      // Extract what data we can.
      //
      {
         this->context.attributes    = type.attributes();
         this->context.attributes_of = type;
         _pull_option_set(this->seen_options.type);
      }
      {
         this->context.attributes    = this->field.attributes();
         this->context.attributes_of = this->field;
         _pull_option_set(this->seen_options.field);
      }
      
      //
      // Coalesce the options and check for contradictions.
      //
      
      coalescing_options coalescing;
      
      this->context.attributes_of = type;
      if (auto* heritable = this->seen_options.type.inherit) {
         _coalesce_heritable(coalescing, *heritable);
      }
      _coalesce_attributes(coalescing, this->seen_options.type);
      
      this->context.attributes_of = this->field;
      if (auto* heritable = this->seen_options.field.inherit) {
         _coalesce_heritable(coalescing, *heritable);
      }
      _coalesce_attributes(coalescing, this->seen_options.field);
      
      //
      // Check that the transform functions match.
      //
      static_assert(false, "TODO");
      
      //
      // Apply the options.
      //
      
      if (coalescing.omit)
         this->result.omit_from_bitpacking = true;
      
      if (coalescing.as_opaque_buffer) {
         this->result.kind = member_kind::buffer;
         this->result.data.emplace<data_options::buffer_data>().bytecount = type.size_in_bytes();
      } else if (const auto* casted = std::get_if<in_progress_options::integral>(&coalescing.data)) {
         auto& src = *casted;
         auto& dst = this->result.data.emplace<data_options::integral_data>();
         try {
            dst = src.bake(this->field, type);
         } catch (std::runtime_error& ex) {
            _report_error(ex.what());
         }
      } else if (const auto* casted = std::get_if<in_progress_options::string>(&coalescing.data)) {
         auto& src = *casted;
         auto& dst = this->result.data.emplace<data_options::string_data>();
         try {
            dst = src.bake(global, this->field, type);
         } catch (std::runtime_error& ex) {
            _report_error(ex.what());
         }
      }
      // Done!
   }
}

namespace bitpacking {
   void data_options::_from_heritable(std::string_view name);
   
   void data_options::_from_attributes(
      const global_options::computed& global,
      gcc_wrappers::decl::field       decl,
      gcc_wrappers::attribute_list    attributes,
      tree attributes_of
   ) {
      auto _location = [attributes_of]() -> location_t {
         return subject_location(attributes_of);
      };
      auto _subject = [attributes_of]() -> std::string {
         return subject_to_string(attributes_of);
      };
      auto _report_warning = [&_location, &_subject]<typename... Args>(const char* fmt, Args&&... args) {
         std::string message = "unusual bitpacking options for %<";
         message += _subject();
         message += "%>: ";
         message += fmt;
         warning_at(_location(), OPT_Wpragmas, message.c_str(), args...);
      };
      auto _report_error = [&_location, &_subject]<typename... Args>(const char* fmt, Args&&... args) {
         std::string message = "invalid bitpacking options for %<";
         message += _subject();
         message += "%>: ";
         message += fmt;
         error_at(_location(), message.c_str(), args...);
      };
      
      //
      // Handle heritable options first.
      //
      if (auto attr = attributes.get_attribute("lu_bitpack_inherit"); !attr.empty()) {
         if (args.has_leading_identifier()) {
            _report_error(
               "%<%s%> attribute must not have an identifier as its first argument.",
               attr.name().data()
            );
         } else {
            std::string name;
            {
               bool fail = true;
               auto args = attr.arguments();
               if (!args.empty()) {
                  auto arg = args.front().as<gw::expr::string_constant>;
                  if (!arg.empty()) {
                     fail = false;
                     name = arg.value();
                     if (name.empty()) {
                        _report_error("%<lu_bitpack_inherit%> heritable options name cannot be blank");
                     }
                  }
               }
               if (fail) {
                  _report_error("%<lu_bitpack_inherit%> attribute must have a string constant argument identifying the set of heritable options to use");
               }
            }
            if (!name.empty()) {
               this->_from_heritable(name);
            }
         }
      }
      
      //
      // Handle other attributes.
      //
      for(auto attr : attributes) {
         auto key  = attr.name();
         auto args = attr.arguments();
         if (key == "lu_bitpack_inherit") {
            //
            // Already handled, above.
            //
            continue;
         }
         
         if (key == "lu_bitpack_omit") {
            this->omit_from_bitpacking = true;
            continue;
         }
         if (key == "lu_bitpack_as_opaque_buffer") {
            if (this->is_explicit_string) {
               _report_error("%<lu_bitpack_as_opaque_buffer%> attribute specified on a field that has already been marked as a string");
            }
            this->is_explicit_buffer = true;
            continue;
         }
         
         
      }
      
      static_assert(false, "TODO");
   }
   
   void data_options::from_field(
      const global_options::computed& global,
      gcc_wrappers::decl::field       decl
   ) {
      static_assert(false, "TODO");
   }
}