#include "codegen/serialization_value_path.h"
#include "codegen/descriptors.h"
#include "lu/strings/printf_string.h"

namespace codegen {
   void serialization_value_path::access_member(const member_descriptor& member) {
      this->access_member(member.decl.name());
   }
   void serialization_value_path::access_member(std::string_view name) {
      this->path += '.';
      this->path += name;
   }
   void serialization_value_path::access_array_element(size_t n) {
      this->access_array_slice(n, 1);
   }
   void serialization_value_path::access_array_slice(size_t start, size_t count) {
      if (count == 1) {
         this->path += lu::strings::printf_string("[%u]", start);
      } else {
         this->path += lu::strings::printf_string("[%u:%u]", start, start + count);
      }
   }
   
   serialization_value_path serialization_value_path::to_member(const member_descriptor& member) {
      return this->to_member(member.decl.name());
   }
   serialization_value_path serialization_value_path::to_member(std::string_view name) const {
      auto copy = *this;
      copy.access_member(name);
      return copy;
   }
   serialization_value_path serialization_value_path::to_array_element(size_t n) const {
      auto copy = *this;
      copy.access_array_element(n);
      return copy;
   }
   serialization_value_path serialization_value_path::to_array_slice(size_t start, size_t count) const {
      auto copy = *this;
      copy.access_array_slice(start, count);
      return copy;
   }
   
   serialization_value_path serialization_value_path::transform_to(
      const std::string& transformed_type_name
   ) const {
      auto copy = *this;
      copy.path = "((";
      copy.path += transformed_type_name;
      copy.path += ')';
      copy.path += this->path;
      copy.path += ')';
      return copy;
   }
}