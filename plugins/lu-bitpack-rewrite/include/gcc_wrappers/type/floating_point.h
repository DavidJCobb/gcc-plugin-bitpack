#pragma once
#include "gcc_wrappers/type/numeric.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::type {
   class floating_point : public numeric {
      public:
         static bool node_is(tree t) {
            return t != NULL_TREE && TREE_CODE(t) != REAL_TYPE;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(floating_point)
         
      public:
         size_t bitcount() const; // TYPE_PRECISION: number of meaningful bits
         
         bool is_single_precision() const;
         bool is_double_precision() const;
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"