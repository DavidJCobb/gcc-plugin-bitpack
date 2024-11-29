#include "xmlgen/report/c_type.h"
#include "gcc_wrappers/decl/type_def.h"

namespace xmlgen::report {
   c_type::c_type(gcc_wrappers::type::base type) : type(type) {
      this->as_xml = std::make_unique<xml_element>();
      auto& node = *this->as_xml;
      
      // Type keyword as node name.
      if (type.is_record()) {
         node.node_name = "struct";
      } else if (type.is_union()) {
         node.node_name = "union";
      } else {
         node.node_name = "unknown-type";
      }
      
      // Typename.
      if (type.is_record() || type.is_union()) {
         auto name = type.name();
         if (!name.empty())
            node.set_attribute("tag", name);
         
         auto decl = type.declaration();
         if (!decl.empty())
            node.set_attribute("name", decl.name());
      } else {
         node.set_attribute("name", type.pretty_print());
      }
   }
   
   void c_type::update_stats_xml() {
      if (auto* prior = this->as_xml->first_child_by_node_name("stats")) {
         this->as_xml->remove_child(*prior);
      }
      
      if (this->c_info.align_of <= 0) {
         this->as_xml->remove_attribute("c-alignment");
      } else {
         this->as_xml->set_attribute_i("c-alignment", this->c_info.align_of);
      }
      if (this->c_info.size_of <= 0) {
         this->as_xml->remove_attribute("c-sizeof");
      } else {
         this->as_xml->set_attribute_i("c-sizeof", this->c_info.size_of);
      }
      
      if (this->stats_data.empty())
         return;
      auto node_ptr = this->stats_data.to_xml();
      this->as_xml->append_child(std::move(node_ptr));
   }
}