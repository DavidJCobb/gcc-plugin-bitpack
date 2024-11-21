#pragma once
#include <cstdint>
#include <optional>
#include <variant>
#include "bitpacking/member_kind.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/array.h"

namespace bitpacking::global_options {
   class computed;
}
namespace gcc_wrappers {
   namespace decl {
      class field;
   }
   namespace type {
      class base;
   }
}

namespace bitpacking::data_options {
   namespace computed_x_options {
      struct buffer {
         size_t bytecount = 0;
         
         constexpr bool operator==(const buffer&) const noexcept = default;
      };
      struct integral { // also boolean, pointer
         size_t    bitcount = 0;
         intmax_t  min = 0;
         uintmax_t max = 0;
         
         constexpr bool operator==(const integral&) const noexcept = default;
      };
      struct string {
         size_t length    = 0;
         bool   nonstring = false;
         
         constexpr bool operator==(const string&) const noexcept = default;
      };
      struct tagged_union {
         std::string tag_identifier;
         bool        is_internal = false;
         
         bool operator==(const tagged_union&) const noexcept = default;
      };
      struct transforms {
         gcc_wrappers::type::base     transformed_type;
         gcc_wrappers::decl::function pre_pack;
         gcc_wrappers::decl::function post_unpack;
         
         bool operator==(const transforms& other) const noexcept {
            if (this->transformed_type.as_untyped() != other.transformed_type.as_untyped())
               return false;
            if (this->pre_pack.as_untyped() != other.pre_pack.as_untyped())
               return false;
            if (this->post_unpack.as_untyped() != other.post_unpack.as_untyped())
               return false;
            return true;
         }
      };
   }
   
   namespace requested_x_options {
      struct buffer {
      };
      struct integral {
         std::optional<size_t> bitcount;
         std::optional<size_t> min;
         std::optional<size_t> max;
         
         constexpr bool empty() const noexcept {
            return !bitcount.has_value() && !min.has_value() && !max.has_value();
         }
         
         void coalesce(const integral& higher_prio);
         
         computed_x_options::integral bake(gcc_wrappers::type::base, member_kind& out_kind, std::optional<size_t> bitfield_width = {}) const;
         computed_x_options::integral bake(gcc_wrappers::decl::field, member_kind& out_kind) const;
      };
      struct string {
         std::optional<bool> nonstring;
         
         computed_x_options::string bake(gcc_wrappers::type::array) const;
      };
      struct tagged_union {
         std::string tag_identifier;
         bool        is_external = false;
         bool        is_internal = false;
      };
      using transforms = computed_x_options::transforms;
   }
}