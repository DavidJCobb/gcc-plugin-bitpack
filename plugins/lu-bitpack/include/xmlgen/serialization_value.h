#pragma once
#include <string>
#include <variant>
#include "codegen/descriptors.h"

namespace xmlgen {
   struct serialization_value {
      std::variant<
         std::monostate,
         const codegen::struct_descriptor*,
         codegen::member_descriptor_view
      > descriptor;
      std::string path;
      
      serialization_value access_member(const codegen::member_descriptor&) const;
      serialization_value access_nth(size_t start, size_t count) const;
      serialization_value access_nth(size_t n) const;
      
      size_t bitcount() const;
      bool is_array() const;
      bool is_struct() const;
      
      // assert(is_array());
      size_t array_extent() const;
      
      constexpr bool is_top_level_struct() const noexcept {
         return std::holds_alternative<const codegen::struct_descriptor*>(this->descriptor);
      }
      constexpr const codegen::struct_descriptor& as_top_level_struct() const noexcept {
         assert(is_top_level_struct());
         return *std::get<const codegen::struct_descriptor*>(this->descriptor);
      }
      
      constexpr bool is_member() const noexcept {
         return std::holds_alternative<codegen::member_descriptor_view>(this->descriptor);
      }
      constexpr codegen::member_descriptor_view& as_member() {
         return std::get<codegen::member_descriptor_view>(this->descriptor);
      }
      constexpr const codegen::member_descriptor_view& as_member() const noexcept {
         return std::get<codegen::member_descriptor_view>(this->descriptor);
      }
   };
}