#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "gcc_wrappers/type/record.h"
#include "bitpacking/global_options.h"
#include "codegen/descriptors.h"
#include "xmlgen/serialization_value.h"
#include "xmlgen/xml_element.h"

namespace xmlgen {
   class sector_xml_generator {
      protected:
         struct whole_struct_data {
            std::unique_ptr<codegen::struct_descriptor> descriptor;
            std::unique_ptr<xml_element> root;
         };
         
         std::unique_ptr<xml_element> _make_whole_data_member(const codegen::member_descriptor&);
         
         void _make_heritable_xml(std::string_view);
         
         void _on_struct_seen(gcc_wrappers::type::record);
         void _on_struct_seen(const serialization_value&);
         const codegen::struct_descriptor* _descriptor_for_struct(const serialization_value&);
         
         struct in_progress_sector {
            sector_xml_generator& owner;
            std::unique_ptr<xml_element> root;
            //
            size_t id             = 0;
            size_t bits_remaining = 0;
            
            bool empty() const;
            void next();
         };
         
         std::unique_ptr<xml_element> _serialize_array_slice(
            const serialization_value&,
            size_t start,
            size_t count
         );
         std::unique_ptr<xml_element> _serialize(const serialization_value&);
         
         void _serialize_value_to_sector(in_progress_sector&, const serialization_value&);
      
      public:
         sector_xml_generator(bitpacking::global_options::computed& go) : global_options(go) {}
      
         // inputs:
         bitpacking::global_options::computed& global_options;
         std::vector<std::vector<std::string>> identifiers_to_serialize;
         
         // outputs
         std::vector<std::unique_ptr<xml_element>> per_sector_xml;
         std::unordered_map<std::string, std::unique_ptr<xml_element>> heritables;
         std::unordered_map<gcc_wrappers::type::record, whole_struct_data> whole_struct_xml;
         
         void run();
         std::string bake() const;
   };
}