#pragma once
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::type {
   class record : public container {
      public:
         static bool node_is(tree t) {
            return t != NULL_TREE && TREE_CODE(t) == RECORD_TYPE;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(record)
         
      public:
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"