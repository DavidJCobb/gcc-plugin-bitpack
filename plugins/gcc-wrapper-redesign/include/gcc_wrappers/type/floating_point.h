#pragma once
#include "gcc_wrappers/type/numeric.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers::type {
   class floating_point : public numeric {
      public:
         static bool node_is(tree t) {
            return TREE_CODE(t) != REAL_TYPE;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(floating_point)
         
      public:
         size_t bitcount() const; // TYPE_PRECISION: number of meaningful bits
         
         bool is_single_precision() const;
         bool is_double_precision() const;
   };
   using floating_point_ptr = node_pointer_template<floating_point>;
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"