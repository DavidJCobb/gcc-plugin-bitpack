#pragma once
#include <cstdint>
#include <optional>
#include <variant>
#include "bitpacking/member_kind.h"

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
      };
      struct integral { // also boolean, pointer
         size_t    bitcount = 0;
         intmax_t  min = 0;
         uintmax_t max = 0;
      };
      struct string {
         size_t length = 0;
         bool   with_terminator = false;
      };
   }
   
   namespace requested_x_options {
      struct integral {
         std::optional<size_t> bitcount;
         std::optional<size_t> min;
         std::optional<size_t> max;
         
         constexpr bool empty() const noexcept {
            return !bitcount.has_value() && !min.has_value() && !max.has_value();
         }
         
         void coalesce(const integral& higher_prio);
         
         // throws std::runtime_error on failure
         computed_x_options::integral bake(
            const bitpacking::global_options::computed&,
            gcc_wrappers::decl::field,
            gcc_wrappers::type::base,
            member_kind& out_kind
         ) const;
      };
      struct string {
         std::optional<size_t> length;
         std::optional<bool>   with_terminator;
         
         void coalesce(const string& higher_prio);
         
         // throws std::runtime_error on failure
         computed_x_options::string bake(
            const bitpacking::global_options::computed&,
            gcc_wrappers::decl::field,
            gcc_wrappers::type::base
         ) const;
      };
      
      using variant = std::variant<
         std::monostate,
         integral,
         string
      >;
   }
}