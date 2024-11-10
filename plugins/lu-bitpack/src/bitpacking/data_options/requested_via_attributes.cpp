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
         result += decl.fully_qualified_name();
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
   
   static std::string_view _pull_heritable_name(tree report_errors_on, gw::attribute_list attributes) {
      auto attr = attributes.get_attribute("lu_bitpack_inherit");
      if (attr.empty())
         return {};
      auto arg  = attr.arguments().front().as<gw::expr::string_constant>();
      auto name = arg.value_view();
      return name;
   }

}

namespace bitpacking::data_options {
   void requested_via_attributes::_load_impl(tree subject, gw::attribute_list attributes) {
      {
         auto name = _pull_heritable_name(subject, attributes);
         if (!name.empty())
            this->inherit = heritable_options_stockpile::get().options_by_name(name);
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
         if (key == "lu_bitpack_default_value") {
            this->default_value_node = attr.arguments().front().as_untyped();
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
            
            if (attributes.has_attribute("nonstring")) {
               dst_data.with_terminator = false;
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