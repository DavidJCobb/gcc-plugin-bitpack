#include "xmlgen/referenceable_struct_members_to_xml.h"
#include "xmlgen/bitpacking_default_value_to_xml.h"
#include "codegen/decl_descriptor.h"
#include "codegen/decl_dictionary.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/type_def.h"
namespace gw {
   using namespace gcc_wrappers;
}

#include "xmlgen/bitpacking_default_value_to_xml.h"
#include "xmlgen/bitpacking_x_options_to_xml.h"

namespace xmlgen {
   extern std::unique_ptr<xml_element> referenceable_struct_members_to_xml(
      gw::type::container type
   ) {
      auto& dict = codegen::decl_dictionary::get();
      
      auto  node_ptr = std::make_unique<xml_element>();
      auto& node     = *node_ptr;
      node.node_name = "members";
      
      type.for_each_referenceable_field([&dict, &node](gw::decl::field field) {
         bool is_bespoke_struct_type = false; // i.e. `struct {} foo;`
         
         auto  child_ptr = std::make_unique<xml_element>();
         auto& child     = *child_ptr;
         child.node_name = "field";
         child.set_attribute("name", field.name());
         {
            auto type = field.value_type();
            auto pp   = type.pretty_print();
            if (pp != "<unnamed>") {
               child.set_attribute("type", pp);
            } else {
               if (type.is_container()) {
                  is_bespoke_struct_type = true;
               }
            }
            if (type.is_integral()) {
               child.set_attribute_b("type-is-signed", type.as_integral().is_signed());
            }
         }
         {  // Bitpacking info and options
            const auto& desc = dict.describe(field);
            for(auto size : desc.array.extents) {
               auto  elem_ptr = std::make_unique<xml_element>();
               auto& elem     = *elem_ptr;
               elem.node_name = "array-rank";
               elem.set_attribute_i("extent", size);
               child.append_child(std::move(elem_ptr));
            }
            const auto& options = desc.options;
            bitpacking_default_value_to_xml(child, options);
            bitpacking_x_options_to_xml(child, options, false);
         }
         if (is_bespoke_struct_type) {
            auto members_ptr = referenceable_struct_members_to_xml(field.value_type().as_container());
            child.append_child(std::move(members_ptr));
         }
         node.append_child(std::move(child_ptr));
      });
      
      return node_ptr;
   }
}