#include "bitpacking/data_options/requested_via_attributes.h"
#include <stdexcept>
#include <vector>
#include "lu/strings/handle_kv_string.h"
#include "bitpacking/heritable_options.h"
#include "bitpacking/transform_function_validation_helpers.h"
#include "gcc_wrappers/decl/type_def.h"
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
   
   static void _warn_on_extra_attribute_args(tree subject, gw::attribute attr, size_t expected) {
      auto argcount = attr.arguments().size();
      if (argcount <= expected)
         return;
      _report_warning(
         subject,
         "%<%s%> attribute specifies more (%u) arguments than expected (%u)",
         attr.name().data(),
         (unsigned int)argcount,
         (unsigned int)expected
      );
   }
   
   static std::string _pull_heritable_name(tree report_errors_on, gw::attribute_list attributes) {
      auto attr = attributes.get_attribute("lu_bitpack_inherit");
      if (attr.empty())
         return {};
      auto args = attr.arguments();
      if (args.empty()) {
         _report_error(report_errors_on,"%<lu_bitpack_inherit%> attribute must have a string constant argument identifying the set of heritable options to use");
         return {};
      }
      _warn_on_extra_attribute_args(report_errors_on, attr, 1);
      
      auto arg = args.front().as<gw::expr::string_constant>();
      if (arg.empty()) {
         _report_error(report_errors_on,"%<lu_bitpack_inherit%> attribute must have a string constant argument identifying the set of heritable options to use");
         return {};
      }
      auto name = arg.value();
      if (name.empty()) {
         _report_error(report_errors_on,"%<lu_bitpack_inherit%> heritable options name cannot be blank");
      }
      return name;
   }

}

namespace bitpacking::data_options {
   void requested_via_attributes::_load_transforms(tree subject, gw::attribute attr) {
      std::string id_pre_pack;
      std::string id_post_unpack;
      
      auto args = attr.arguments();
      if (!args.empty()) {
         _warn_on_extra_attribute_args(subject, attr, 1);
         auto arg = args.front().as<gw::expr::string_constant>();
         if (arg.empty()) {
            _report_error(
               subject,
               "%<lu_bitpack_funcs%> attribute argument must be a string literal of "
               "the form \"pre_pack=identifier,post_unpack=identifier\""
            );
            return;
         }
         auto data = arg.value();
         try {
            lu::strings::handle_kv_string(data, std::array{
               lu::strings::kv_string_param{
                  .name      = "pre_pack",
                  .has_param = true,
                  .handler   = [&id_pre_pack](std::string_view v, int vi) {
                     id_pre_pack = v;
                  },
               },
               lu::strings::kv_string_param{
                  .name      = "post_unpack",
                  .has_param = false,
                  .handler   = [&id_post_unpack](std::string_view v, int vi) {
                     id_post_unpack = v;
                  },
               },
            });
         } catch (std::runtime_error& ex) {
            _report_error(
               subject,
               "%<lu_bitpack_funcs%> attribute failed to parse: %s",
               ex.what()
            );
            return;
         }
      }
      
      if (id_pre_pack.empty() != id_post_unpack.empty()) {
         if (id_pre_pack.empty()) {
            _report_error(subject, "%<lu_bitpack_funcs%> attribute specifies only a post-unpack function; you must specify both pre-pack and post-unpack functions");
         } else {
            _report_error(subject, "%<lu_bitpack_funcs%> attribute specifies only a pre-pack function; you must specify both pre-pack and post-unpack functions");
         }
         return;
      }
      
      bool each_is_individually_correct = true;
      
      auto decl_pre  = this->transforms.pre_pack    = _get_func_decl(id_pre_pack.c_str());
      auto decl_post = this->transforms.post_unpack = _get_func_decl(id_post_unpack.c_str());
      if (decl_pre.empty()) {
         _report_error(
            subject,
            "%<lu_bitpack_funcs%> attribute specifies a pre-pack identifier %<%s%> that "
            "either doesn't exist or is not a function",
            id_pre_pack.c_str()
         );
      } else {
         if (!can_be_pre_pack_function(decl_pre)) {
            each_is_individually_correct = false;
            
            _report_error(subject, "%<lu_bitpack_funcs%> attribute specifies a pre-pack function with the wrong signature; signature should be <%void %s(const NormalType*, PackedType*)%>.", decl_pre.name().data());
         }
      }
      if (decl_post.empty()) {
         _report_error(
            subject,
            "%<lu_bitpack_funcs%> attribute specifies a post-unpack identifier %<%s%> that "
            "either doesn't exist or is not a function",
            id_post_unpack.c_str()
         );
      } else {
         if (!can_be_post_unpack_function(decl_post)) {
            each_is_individually_correct = false;
            
            _report_error(subject, "%<lu_bitpack_funcs%> attribute specifies a post-unpack function with the wrong signature; signature should be <%void %s(NormalType*, const PackedType*)%>.", decl_post.name().data());
         }
      }
      
      if (!each_is_individually_correct) {
         return;
      }
      if (decl_pre.empty() || decl_post.empty()) {
         return;
      }
      
      // TODO: Move this check to after we've resolved heritables, etc.
      auto problem = check_transform_functions_match_types(decl_pre, decl_post);
      if (!problem.empty()) {
         problem = std::string("%<lu_bitpack_funcs%> attribute specifies mismatched functions: ") + problem;
         _report_error(subject, problem.c_str());
      }
   }
   
   void requested_via_attributes::_load_impl(tree subject, gw::attribute_list attributes) {
      {
         auto name = _pull_heritable_name(subject, attributes);
         if (!name.empty()) {
            auto* o = heritable_options_stockpile::get().options_by_name(name);
            if (!o) {
               _report_error(subject, "%<lu_bitpack_inherit%>: no heritable option set exists with the name %<%s%>", name.c_str());
            }
            this->inherit = o;
         }
      }
      
      for(auto attr : attributes) {
         auto key = attr.name();
         if (key == "lu_bitpack_inherit") {
            //
            // Already handled.
            //
            continue;
         }
         
         if (key == "lu bitpack invalid attributes") {
            this->failed = true;
            continue;
         }
         
         //
         // Attribute arguments will have been validated in advance: GCC 
         // verified that the attributes have the right number of args, 
         // and our attribute handlers rejected any obviously malformed 
         // args. We can just access things blindly.
         //
         
         if (key == "lu_bitpack_omit") {
            this->omit = true;
            continue;
         }
         if (key == "lu_bitpack_as_opaque_buffer") {
            this->as_opaque_buffer = true;
            continue;
         }
         if (key == "lu_bitpack_bitcount") {
            this->x_options.integral.bitcount = attr.arguments().front().as<gw::expr::integer_constant>().value<intmax_t>();
            continue;
         }
         if (key == "lu_bitpack_funcs") {
            auto args   = attr.arguments();
            auto node_a = args[0];
            auto node_b = args[1];
            if (!node_a.empty()) {
               this->transforms.pre_pack = node_a.as<gw::decl::function>();
            }
            if (!node_b.empty()) {
               this->transforms.post_unpack = node_b.as<gw::decl::function>();
            }
            continue;
         }
         if (key == "lu_bitpack_range") {
            auto args = attr.arguments();
            this->x_options.integral.min = args[0].as<gw::expr::integer_constant>().value<intmax_t>();
            this->x_options.integral.max = args[1].as<gw::expr::integer_constant>().value<intmax_t>();
            continue;
         }
         if (key == "lu_bitpack_string") {
            auto& dst_opt  = this->x_options.string;
            auto& dst_data = dst_opt.has_value() ? *dst_opt : dst_opt.emplace();
            
            auto args = attr.arguments();
            if (!args.empty()) {
               auto node_a = args[0];
               auto node_b = args[1];
               if (!node_a.empty()) {
                  dst_data.length = node_a.as<gw::expr::integer_constant>().value<size_t>().value();
               }
               if (!node_b.empty()) {
                  dst_data.with_terminator = node_b.as_untyped() == boolean_true_node;
               }
            }
            continue;
         }
         
         if (key.starts_with("lu_bitpack_")) {
            _report_warning(subject, "unrecognized bitpacking attribute %<%s%>", key.data());
         }
      }
   }
    
   bool requested_via_attributes::load(gcc_wrappers::decl::field decl) {
      auto errors_prior = errorcount; // diagnostic.h
      _load_impl(decl.as_untyped(), decl.attributes());
      auto errors_after = errorcount;
      return !this->failed && errors_prior == errors_after;
      
   }
   bool requested_via_attributes::load(gcc_wrappers::type::base type) {
      auto errors_prior = errorcount; // diagnostic.h
      
      auto decl = type.declaration();
      if (!decl.empty() && !decl.is_synonym_of().empty()) {
         //
         // Handle transitive typedefs. For example, given `typedef a b`, we want 
         // to apply bitpacking options for `a` when we see `b`, before we apply 
         // any bitpacking options for `b`.
         //
         std::vector<gw::type::base> transitives;
         transitives.push_back(type);
         do {
            auto tran = decl.is_synonym_of();
            if (tran.empty())
               break;
            transitives.push_back(tran);
            decl = tran.declaration();
         } while (!decl.empty());
         for(auto it = transitives.rbegin(); it != transitives.rend(); ++it) {
            auto tran = *it;
            auto decl = tran.declaration();
            _load_impl(tran.as_untyped(), tran.attributes());
            _load_impl(tran.as_untyped(), decl.attributes());
         }
      } else {
         _load_impl(type.as_untyped(), type.attributes());
      }
      
      auto errors_after = errorcount;
      return !this->failed && errors_prior == errors_after;
   }
}