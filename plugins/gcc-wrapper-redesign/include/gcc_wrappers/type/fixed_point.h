#pragma once
#include "gcc_wrappers/type/numeric.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers::type {
   class fixed_point : public numeric {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) != FIXED_POINT_TYPE;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(fixed_point)
         
      public:
         size_t fractional_bitcount() const; // TYPE_FBIT
         size_t integral_bitcount() const;   // TYPE_IBIT
         size_t total_bitcount() const;      // TYPE_PRECISION
         
         bool is_saturating() const;
         bool is_signed() const;
         bool is_unsigned() const;
         
         intmax_t minimum_value() const;
         uintmax_t maximum_value() const;
         
         fixed_point make_signed() const;
         fixed_point make_unsigned() const;
   };
   using fixed_point_ptr = node_pointer_template<fixed_point>;
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"