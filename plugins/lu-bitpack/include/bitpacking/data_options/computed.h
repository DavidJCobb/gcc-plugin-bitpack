#pragma once
#include <optional>
#include <variant>
#include "gcc_wrappers/decl/function.h"
#include "bitpacking/data_options/x_options.h"
#include "bitpacking/member_kind.h"

namespace bitpacking::data_options {
   class computed {
      public:
         bool omit_from_bitpacking = false;
         
         member_kind kind = member_kind::none;
         
         std::variant<
            std::monostate,
            computed_x_options::buffer,
            computed_x_options::integral,
            computed_x_options::string
         > data;
         
         struct {
            gcc_wrappers::decl::function pre_pack;
            gcc_wrappers::decl::function post_unpack;
         } transforms;
         
      public:
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