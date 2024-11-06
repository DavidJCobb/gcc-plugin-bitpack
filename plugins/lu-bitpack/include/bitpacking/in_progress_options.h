#pragma once
#include <optional>

namespace bitpacking {
   namespace global_options {
      class computed;
   }
   namespace typed_data_options {
      struct integral;
      struct string;
   }
}
namespace gcc_wrappers {
   namespace decl {
      class field;
   }
   namespace type {
      class base;
   }
}

namespace bitpacking::in_progress_options {
   struct integral {
      std::optional<size_t> bitcount;
      std::optional<size_t> min;
      std::optional<size_t> max;
      
      constexpr bool empty() const noexcept {
         return !bitcount.has_value() && !min.has_value() && !max.has_value();
      }
      
      void coalesce(const integral& higher_prio);
      
      // throws std::runtime_error on failure
      typed_data_options::integral bake(gcc_wrappers::decl::field, gcc_wrappers::type::base) const;
   };
   struct string {
      std::optional<size_t> length;
      std::optional<bool>   with_terminator;
      
      void coalesce(const string& higher_prio);
      
      // throws std::runtime_error on failure
      typed_data_options::string bake(
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