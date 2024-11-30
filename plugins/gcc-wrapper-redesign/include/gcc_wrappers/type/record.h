#pragma once
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers::type {
   class record : public container {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == RECORD_TYPE;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(record)
         
      public:
   };
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"