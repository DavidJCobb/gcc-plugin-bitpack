#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace xmlgen {
   class xml_element {
      public:
         std::string node_name;
         std::unordered_map<std::string, std::string> attributes;
         std::vector<std::unique_ptr<xml_element>> children;
         xml_element* parent = nullptr;
         std::string text_content;
         
      public:
         void append_child(std::unique_ptr<xml_element>&&);
         void remove_child(xml_element&);
         
         // Asserts that the name is a valid name.
         void set_attribute(std::string_view name, std::string_view value);
         
         std::string to_string(size_t indent_level = 0) const;
   };
}