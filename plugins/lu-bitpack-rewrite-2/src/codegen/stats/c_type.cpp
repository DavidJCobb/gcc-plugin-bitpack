#include "codegen/stats/c_type.h"
#include "gcc_wrappers/decl/type_def.h"

using xml_element = xmlgen::xml_element;

namespace codegen::stats {
   c_type::c_type(gcc_wrappers::type::base type) : type(type) {
   }
   
   std::unique_ptr<xml_element> c_type::to_xml() const {
      auto out = std::make_unique<xml_element>();
      if (this->type.is_container()) {
         if (this->type.is_record()) {
            out->node_name = "struct";
         } else if (this->type.is_union()) {
            out->node_name = "union";
         }
         
         auto name = this->type.name();
         if (!name.empty())
            out->set_attribute("tag", name);
         
         auto decl = this->type.declaration();
         if (decl)
            out->set_attribute("name", decl->name());
      } else {
         out->set_attribute("name", this->type.pretty_print());
      }
      if (out->node_name.empty())
         out->node_name = "unknown-type";
      
      if (serializable::empty()) {
         out->append_child(serializable::to_xml());
      }
      
      if (this->c_info.align_of > 0) {
         out->set_attribute_i("c-alignment", this->c_info.align_of);
      }
      if (this->c_info.size_of > 0) {
         out->set_attribute_i("c-sizeof", this->c_info.size_of);
      }
      
      return out;
   }
}