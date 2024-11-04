#pragma once
#include <optional>
#include <variant>
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/list_node.h"
#include "gcc_wrappers/type/base.h"
#include "bitpacking/member_kind.h"

namespace bitpacking::global_options {
   class computed;
}

namespace bitpacking::member_options {
   class requested {
      public:
         bool omit_from_bitpacking = false;
         bool is_explicit_buffer   = false;
         bool is_explicit_string   = false;
         
         std::string inherit_from;
         struct {
            std::string pre_pack;
            std::string post_unpack;
         } transforms;
         struct {
            std::optional<size_t> bitcount;
            std::optional<size_t> min;
            std::optional<size_t> max;
         } integral;
         struct {
            std::optional<size_t> length;
            std::optional<bool>   with_terminator;
         } string;
         
         void from_field(gcc_wrappers::decl::field);
   };
   
   class computed {
      public:
         struct buffer_data {
            size_t bytecount = 0;
         };
         struct integral_data { // also boolean, pointer
            size_t   bitcount = 0;
            intmax_t min = 0;
         };
         struct string_data {
            size_t length = 0;
            bool   with_terminator = false;
         };
      
      public:
         member_kind kind = member_kind::none;
         std::variant<
            std::monostate,
            buffer_data,
            integral_data,
            string_data
         > data;
         struct {
            std::string pre_pack;
            std::string post_unpack;
         } transforms;
         
         // assert(!src.omit_from_bitpacking);
         void resolve(
            const global_options::computed&,
            const requested& src,
            gcc_wrappers::type::base member_type
         );
         
         constexpr bool is_buffer() const noexcept {
            return std::holds_alternative<buffer_data>(this->data);
         }
         constexpr bool is_integral() const noexcept {
            return std::holds_alternative<integral_data>(this->data);
         }
         constexpr bool is_string() const noexcept {
            return std::holds_alternative<string_data>(this->data);
         }
         
         constexpr const buffer_data& buffer_options() const noexcept {
            return std::get<buffer_data>(this->data);
         }
         constexpr const integral_data& integral_options() const noexcept {
            return std::get<integral_data>(this->data);
         }
         constexpr const string_data& string_options() const noexcept {
            return std::get<string_data>(this->data);
         }
         //
         // non-const:
         //
         constexpr buffer_data& buffer_options() noexcept {
            return std::get<buffer_data>(this->data);
         }
         constexpr integral_data& integral_options() noexcept {
            return std::get<integral_data>(this->data);
         }
         constexpr string_data& string_options() noexcept {
            return std::get<string_data>(this->data);
         }
   };
}