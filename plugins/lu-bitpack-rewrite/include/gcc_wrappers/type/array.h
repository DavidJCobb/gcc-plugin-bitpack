#pragma once
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::type {
   class array : public base {
      public:
         static bool node_is(tree t) {
            return t != NULL_TREE && TREE_CODE(t) == ARRAY_TYPE;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(array)
         
      public:
         bool is_variable_length_array() const;
         
         // returns empty if a variable-length array.
         std::optional<size_t> extent() const;
         
         // 1 for foo[n]; 2 for foo[m][n]; etc.
         size_t rank() const;
         
         base value_type() const;
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"