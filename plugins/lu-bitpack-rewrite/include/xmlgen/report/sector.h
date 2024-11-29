#pragma once
#include "xmlgen/report/stats.h"
#include "xmlgen/xml_element.h"

namespace xmlgen::report {
   class sector {
      public:
         sector();
      
      public:
         std::unique_ptr<xml_element> as_xml;
         stats stats_data;
         
         void update_stats_xml();
   };
}