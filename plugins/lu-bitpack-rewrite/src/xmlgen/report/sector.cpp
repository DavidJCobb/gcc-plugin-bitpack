#include "xmlgen/report/sector.h"

namespace xmlgen::report {
   sector::sector() {
      this->as_xml = std::make_unique<xml_element>();
      this->as_xml->node_name = "sector";
   }
   
   void sector::update_stats_xml() {
      if (auto* prior = this->as_xml->first_child_by_node_name("stats")) {
         this->as_xml->remove_child(*prior);
      }
      if (this->stats_data.empty())
         return;
      auto node_ptr = this->stats_data.to_xml();
      this->as_xml->append_child(std::move(node_ptr));
   }
}