#pragma once
#include <optional>
#include <type_traits>
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/type.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      class integer_constant : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == INTEGER_CST;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(integer_constant)
            
            using host_wide_int_type  = HOST_WIDE_INT;
            using host_wide_uint_type = std::make_unsigned_t<host_wide_int_type>;
         
         public:
            integer_constant(type, int);
            
            type value_type() const;
            
            std::optional<host_wide_int_type>  try_value_signed() const;
            std::optional<host_wide_uint_type> try_value_unsigned() const;
            
            // Returns empty if the value is not representable in the given integer type.
            template<typename Integer> requires (std::is_integral_v<Integer> && std::is_arithmetic_v<Integer>)
            std::optional<Integer> value() const {
               using intermediate_type = std::conditional_t<
                  std::is_signed_v<Integer>,
                  host_wide_int_type,
                  host_wide_uint_type
               >;
               
               std::optional<intermediate_type> opt;
               if constexpr (std::is_signed_v<Integer>) {
                  opt = try_value_signed();
               } else {
                  opt = try_value_unsigned();
               }
               if (!opt.has_value())
                  return {};
               
               intermediate_type v = *opt;
               if constexpr (!std::is_same_v<Integer, intermediate_type>) {
                  if constexpr (std::is_signed_v<Integer>) {
                     if (v < std::numeric_limits<Integer>::lowest())
                        return {};
                  }
                  if (v > std::numeric_limits<Integer>::max())
                     return {};
               }
               return (Integer) v;
            }
      };
      static_assert(sizeof(integer_constant) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"