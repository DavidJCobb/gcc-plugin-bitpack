#pragma once
#include <string>
#include <variant>
#include "codegen/descriptors.h"
#include "codegen/value_pair.h"

namespace codegen {
   struct serialization_value : public value_pair {
      std::variant<
         std::monostate,
         const struct_descriptor*,
         member_descriptor_view
      > descriptor;
      
      void assert_valid() const;
      std::string describe() const;
      
      // accessors on structs:
      serialization_value access_member(const member_descriptor&);
      
      // accessors on arrays:
      serialization_value access_array_slice(value_pair n);
      serialization_value access_nth(size_t n);
      
      size_t bitcount() const;
      bool is_array() const;
      bool is_struct() const;
      bool is_transformed() const;
      
      // assert(is_array());
      size_t array_extent() const;
      
      constexpr bool is_top_level_struct() const noexcept {
         return std::holds_alternative<const struct_descriptor*>(this->descriptor);
      }
      constexpr const struct_descriptor& as_top_level_struct() const noexcept {
         assert(is_top_level_struct());
         return *std::get<const struct_descriptor*>(this->descriptor);
      }
      
      constexpr bool is_member() const noexcept {
         return std::holds_alternative<member_descriptor_view>(this->descriptor);
      }
      constexpr member_descriptor_view& as_member() {
         return std::get<member_descriptor_view>(this->descriptor);
      }
      constexpr const member_descriptor_view& as_member() const noexcept {
         return std::get<member_descriptor_view>(this->descriptor);
      }
   };
}