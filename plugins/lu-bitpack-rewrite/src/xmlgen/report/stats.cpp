#include "xmlgen/report/stats.h"

namespace xmlgen::report {
   bool stats::empty() const {
      if (this->counts.total > 0)
         return false;
      if (this->bitcounts.total_packed.has_value())
         return false;
      if (this->bitcounts.total_unpacked.has_value())
         return false;
      if (!this->counts.by_sector.empty())
         return false;
      if (!this->counts.by_top_level_identifier.empty())
         return false;
      return true;
   }
   
   std::unique_ptr<xml_element> stats::to_xml() const {
      auto  node_ptr = std::make_unique<xml_element>();
      auto& node     = *node_ptr;
      node.node_name = "stats";
      
      if (this->bitcounts.total_unpacked.has_value() || this->bitcounts.total_packed.has_value()) {
         auto  child_ptr = std::make_unique<xml_element>();
         auto& child     = *child_ptr;
         child.node_name = "bitcounts";
         node.append_child(std::move(child_ptr));
         
         if (auto& opt = this->bitcounts.total_unpacked; opt.has_value()) {
            child.set_attribute_i("total-unpacked", *opt);
         }
         if (auto& opt = this->bitcounts.total_packed; opt.has_value()) {
            child.set_attribute_i("total-packed", *opt);
         }
      }
      
      if (this->counts.total > 0 || !this->counts.by_sector.empty() || !this->counts.by_top_level_identifier.empty()) {
         auto  child_ptr = std::make_unique<xml_element>();
         auto& child     = *child_ptr;
         child.node_name = "counts";
         node.append_child(std::move(child_ptr));
         
         if (this->counts.total > 0)
            child.set_attribute_i("total", this->counts.total);
         
         if (const auto& list = this->counts.by_sector; !list.empty()) {
            for(const auto& item : list) {
               auto  grandchild_ptr = std::make_unique<xml_element>();
               auto& grandchild     = *grandchild_ptr;
               grandchild.node_name = "in-sector";
               child.append_child(std::move(grandchild_ptr));
               
               grandchild.set_attribute_i("index", item.sector_index);
               grandchild.set_attribute_i("count", item.count);
            }
         }
         if (const auto& list = this->counts.by_top_level_identifier; !list.empty()) {
            for(const auto& item : list) {
               auto  grandchild_ptr = std::make_unique<xml_element>();
               auto& grandchild     = *grandchild_ptr;
               grandchild.node_name = "in-top-level-value";
               child.append_child(std::move(grandchild_ptr));
               
               grandchild.set_attribute("name",  item.identifier);
               grandchild.set_attribute_i("count", item.count);
            }
         }
      }
      
      return node_ptr;
   }
}