#pragma once
#include <variant>
#include "codegen/value_pair.h"

namespace codegen {
   struct struct_descriptor;
   struct member_descriptor;
   struct member_descriptor_view;
}

namespace codegen {
   struct serialization_value : public value_pair {
      std::variant<
         std::monostate,
         const struct_descriptor*,
         member_descriptor_view
      > descriptor;
            
      serialization_value access_member(const member_descriptor&);
      serialization_value access_nth(value_pair n);
      
      size_t bitcount() const;
      bool is_array() const;
      bool is_struct() const;
      
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
         return std::get<struct_descriptor>(this->descriptor);
      }
      constexpr const member_descriptor_view& as_member() const noexcept {
         return std::get<struct_descriptor>(this->descriptor);
      }
   };
}