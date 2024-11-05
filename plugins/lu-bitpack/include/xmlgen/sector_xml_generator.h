#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "xmlgen/xml_element.h"

namespace bitpacking {
   class heritable_options;
}
namespace codegen {
   class member_descriptor;
   class sector_functions_generator;
}

namespace xmlgen {
   class sector_xml_generator {
      protected:
         std::unique_ptr<xml_element> _make_whole_data_member(const codegen::member_descriptor&);
         
         void _make_heritable_xml(std::string_view, const bitpacking::heritable_options&);
      
      public:
         // outputs
         std::vector<
            std::vector<std::unique_ptr<xml_element>>
         > serialized_identifiers;
         std::vector<std::unique_ptr<xml_element>> per_type_xml;
         std::vector<std::unique_ptr<xml_element>> per_sector_xml;
         std::unordered_map<std::string, std::unique_ptr<xml_element>> heritables;
         
         void run(const codegen::sector_functions_generator&);
         std::string bake() const;
   };
}