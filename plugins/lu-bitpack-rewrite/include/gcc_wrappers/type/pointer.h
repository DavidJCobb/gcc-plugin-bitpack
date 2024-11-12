#pragma once
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::type {
   class pointer : public base {
      public:
         static bool node_is(tree t) {
            return t != NULL_TREE && TREE_CODE(t) == POINTER_TYPE;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(pointer)
         
      public:
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"