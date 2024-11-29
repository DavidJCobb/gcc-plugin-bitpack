#pragma once
#include "xmlgen/report/stats.h"
#include "xmlgen/xml_element.h"
#include "gcc_wrappers/type/base.h"

namespace xmlgen::report {
   class c_type {
      public:
         c_type(gcc_wrappers::type::base);
      
      public:
         gcc_wrappers::type::base type;
         
         std::unique_ptr<xml_element> as_xml;
         struct {
            size_t align_of = 0;
            size_t size_of  = 0;
         } c_info;
         stats stats_data;
         
         void update_stats_xml();
   };
}