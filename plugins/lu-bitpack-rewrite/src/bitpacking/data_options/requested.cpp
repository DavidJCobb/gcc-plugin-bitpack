#include "bitpacking/data_options/requested.h"
#include "bitpacking/for_each_influencing_entity.h"
#include "attribute_handlers/helpers/type_transitively_has_attribute.h" // TODO: move this
#include <vector>
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/builtin_types.h"
#include <diagnostic.h> // warning(), etc.
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
}

namespace bitpacking::data_options {
   void requested::_load_impl(tree subject, gw::attribute_list attributes) {
      for(auto attr : attributes) {
         auto key = attr.name();
         
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
         
         // X-Options: buffer
         if (key == "lu_bitpack_as_opaque_buffer") {
            this->x_options.emplace<requested_x_options::buffer>();
            continue;
         }
         
         // X-Options: integral
         if (key == "lu_bitpack_bitcount") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::integral>(dst_var))
               dst_var.emplace<requested_x_options::integral>();
            auto& dst_data = std::get<requested_x_options::integral>(dst_var);
            
            dst_data.bitcount = attr.arguments().front().as<gw::expr::integer_constant>().value<intmax_t>();
            continue;
         }
         if (key == "lu_bitpack_range") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::integral>(dst_var))
               dst_var.emplace<requested_x_options::integral>();
            auto& dst_data = std::get<requested_x_options::integral>(dst_var);
            
            auto args = attr.arguments();
            dst_data.min = args[0].as<gw::expr::integer_constant>().value<intmax_t>();
            dst_data.max = args[1].as<gw::expr::integer_constant>().value<intmax_t>();
            continue;
         }
         
         // X-Options: string
         if (key == "lu_bitpack_string") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::string>(dst_var))
               dst_var.emplace<requested_x_options::string>();
            auto& dst_data = std::get<requested_x_options::string>(dst_var);
            
            bool nonstring = false;
            {
               gw::type::base type;
               if (DECL_P(subject)) {
                  type.set_from_untyped(TREE_TYPE(subject));
                  if (attributes.has_attribute("lu_nonstring") || attributes.has_attribute("nonstring"))
                     nonstring = true;
               } else if (TYPE_P(subject)) {
                  type.set_from_untyped(subject);
               }
               if (!nonstring && !type.empty()) {
                  if (attribute_handlers::helpers::type_transitively_has_attribute(type, "lu_nonstring"))
                     nonstring = true;
               }
            }
            if (nonstring)
               dst_data.nonstring = true;
            
            continue;
         }
         
         // X-Options: transforms
         if (key == "lu_bitpack_transforms") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::transforms>(dst_var))
               dst_var.emplace<requested_x_options::transforms>();
            auto& dst_data = std::get<requested_x_options::transforms>(dst_var);
            
            auto args   = attr.arguments();
            auto node_a = args[0];
            auto node_b = args[1];
            if (!node_a.empty()) {
               dst_data.pre_pack = node_a.as<gw::decl::function>();
               dst_data.transformed_type = dst_data.pre_pack.function_type().nth_argument_type(1).remove_pointer();
            }
            if (!node_b.empty()) {
               dst_data.post_unpack = node_b.as<gw::decl::function>();
               dst_data.transformed_type = dst_data.pre_pack.function_type().nth_argument_type(1).remove_pointer().remove_const();
            }
            continue;
         }
         
         if (key.starts_with("lu_bitpack_")) {
            _report_warning(subject, "unrecognized bitpacking attribute %<%s%>", key.data());
         }
      }
   }
    
   bool requested::load(gw::type::base type) {
      for_each_influencing_entity(type, [this](tree node) {
         if (TYPE_P(node)) {
            _load_impl(node, gw::type::base::from_untyped(node).attributes());
         } else {
            _load_impl(node, gw::decl::base::from_untyped(node).attributes());
         }
      });
      return this->failed;
   }
}