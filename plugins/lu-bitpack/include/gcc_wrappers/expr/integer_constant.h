#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      class integer_constant : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == INTEGER_CST;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(integer_constant)
         
         public:
            integer_constant(type, int);
      };
      static_assert(sizeof(integer_constant) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"