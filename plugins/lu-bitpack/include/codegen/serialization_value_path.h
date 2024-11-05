#pragma once
#include <string>
#include <string_view>

namespace codegen {
   class member_descriptor;
}

namespace codegen {
   class serialization_value_path {
      public:
         std::string path;
         
      public:
         void access_member(const member_descriptor&);
         void access_member(std::string_view name);
         void access_array_element(size_t);
         void access_array_slice(size_t start, size_t count);
         
         serialization_value_path to_member(const member_descriptor&);
         serialization_value_path to_member(std::string_view name) const;
         serialization_value_path to_array_element(size_t) const;
         serialization_value_path to_array_slice(size_t start, size_t count) const;
   };
}