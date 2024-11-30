#pragma once
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers::type {
   class untagged_union : public container {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == UNION_TYPE;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(untagged_union)
         
      public:
   };
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"