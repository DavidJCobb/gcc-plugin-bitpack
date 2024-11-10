#pragma once
#include <optional>
#include <variant>
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/type/base.h"
#include "bitpacking/data_options/x_options.h"
#include "bitpacking/member_kind.h"

namespace bitpacking::data_options {
   class requested;
   
   class computed {
      public:
         bool omit_from_bitpacking = false;
         
         member_kind kind = member_kind::none;
         
         // INTEGER_CST, REAL_CST, or STRING_CST
         tree default_value_node = NULL_TREE;
         
         std::variant<
            std::monostate,
            computed_x_options::buffer,
            computed_x_options::integral,
            computed_x_options::string,
            computed_x_options::transforms
         > data;
         
      public:
         bool load(gcc_wrappers::decl::field);
      
         constexpr bool is_buffer() const noexcept {
            return std::holds_alternative<computed_x_options::buffer>(this->data);
         }
         constexpr bool is_integral() const noexcept {
            return std::holds_alternative<computed_x_options::integral>(this->data);
         }
         constexpr bool is_string() const noexcept {
            return std::holds_alternative<computed_x_options::string>(this->data);
         }
         
         constexpr const computed_x_options::buffer& buffer_options() const noexcept {
            return std::get<computed_x_options::buffer>(this->data);
         }
         constexpr const computed_x_options::integral& integral_options() const noexcept {
            return std::get<computed_x_options::integral>(this->data);
         }
         constexpr const computed_x_options::string& string_options() const noexcept {
            return std::get<computed_x_options::string>(this->data);
         }
         //
         // non-const:
         //
         constexpr computed_x_options::buffer& buffer_options() noexcept {
            return std::get<computed_x_options::buffer>(this->data);
         }
         constexpr computed_x_options::integral& integral_options() noexcept {
            return std::get<computed_x_options::integral>(this->data);
         }
         constexpr computed_x_options::string& string_options() noexcept {
            return std::get<computed_x_options::string>(this->data);
         }
   };
}