#pragma once
#include <cstdint>
#include <limits>
#include <optional>
#include <variant>
#include <version>
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/type/base.h"

namespace bitpacking::typed_data_options {
   namespace requested {
      struct buffer;
      struct integral;
      struct string;
      struct tagged_union;
      struct transformed;
   }
   
   namespace computed {
      struct boolean {
         constexpr bool operator==(const boolean&) const noexcept = default;
      };
      struct buffer {
         size_t bytecount = 0;
         
         bool load(gcc_wrappers::node target, const requested::buffer&, bool complain = true);
         
         constexpr bool operator==(const buffer&) const noexcept = default;
      };
      struct integral {
         static constexpr const uintmax_t no_maximum = std::numeric_limits<uintmax_t>::max();
         
         size_t    bitcount = 0;
         intmax_t  min      = 0;
         uintmax_t max      = no_maximum;
         
         bool load(gcc_wrappers::node target, const requested::integral&, bool complain = true);
         
         constexpr bool operator==(const integral&) const noexcept = default;
      };
      struct pointer {
         constexpr bool operator==(const pointer&) const noexcept = default;
      };
      struct string {
         size_t length    = 0;     // does not include null terminator
         bool   nonstring = false; // if true, we don't need a null terminator (i.e. GCC attr)
         
         bool load(gcc_wrappers::node target, const requested::string&, bool complain = true);
         
         constexpr bool operator==(const string&) const noexcept = default;
      };
      struct tagged_union {
         std::string tag_identifier;
         bool        is_internal = false;
         
         bool load(gcc_wrappers::node target, const requested::tagged_union&, bool complain = true);
         
         #if __cpp_lib_constexpr_string >= 201907L
         constexpr
         #endif
         bool operator==(const tagged_union&) const noexcept = default;
      };
      struct transformed {
         gcc_wrappers::type::optional_base     transformed_type;
         gcc_wrappers::decl::optional_function pre_pack;
         gcc_wrappers::decl::optional_function post_unpack;
         
         bool load(gcc_wrappers::node target, const requested::transformed&, bool complain = true);
         
         bool operator==(const transformed&) const noexcept = default;
      };
   }
   namespace requested {
      struct buffer {
         constexpr bool operator==(const buffer&) const noexcept = default;
      };
      struct integral {
         std::optional<size_t>    bitcount;
         std::optional<intmax_t>  min;
         std::optional<uintmax_t> max;
         
         constexpr bool operator==(const integral&) const noexcept = default;
      };
      struct string {
         std::optional<bool> nonstring;
         
         constexpr bool operator==(const string&) const noexcept = default;
      };
      struct tagged_union {
         std::string tag_identifier;
         bool        is_external = false;
         bool        is_internal = false;
         
         #if __cpp_lib_constexpr_string >= 201907L
         constexpr
         #endif
         bool operator==(const tagged_union&) const noexcept = default;
      };
      struct transformed {
         gcc_wrappers::type::optional_base     transformed_type;
         gcc_wrappers::decl::optional_function pre_pack;
         gcc_wrappers::decl::optional_function post_unpack;
         
         bool operator==(const transformed&) const noexcept = default;
      };
   }
   
   using computed_variant = std::variant<
      std::monostate,
      computed::boolean,
      computed::buffer,
      computed::integral,
      computed::pointer,
      computed::string,
      computed::tagged_union,
      computed::transformed
   >;
   using requested_variant = std::variant<
      std::monostate,
      requested::buffer,
      requested::integral,
      requested::string,
      requested::tagged_union,
      requested::transformed
   >;
}