#pragma once
#include <optional>
#include <string>
#include <vector>
#include "xmlgen/xml_element.h"

namespace xmlgen::report {
   class stats {
      public:
         struct count_in_sector {
            size_t sector_index = 0;
            size_t count        = 0;
         };
         struct count_in_top_level_identifier {
            std::string identifier;
            size_t      count;
         };
      
      public:
         struct {
            std::optional<size_t> total_packed;
            std::optional<size_t> total_unpacked;
         } bitcounts;
         struct {
            size_t total = 0;
            std::vector<count_in_sector> by_sector;
            std::vector<count_in_top_level_identifier> by_top_level_identifier;
         } counts;
         
      public:
         bool empty() const;
         std::unique_ptr<xml_element> to_xml() const;
   };
}