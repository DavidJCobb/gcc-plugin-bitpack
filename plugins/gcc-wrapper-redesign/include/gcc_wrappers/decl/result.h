#pragma once
#include "gcc_wrappers/decl/base_value.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class result : public base_value {
         public:
            static bool raw_node_is(tree t) {
               return TREE_CODE(t) == RESULT_DECL;
            }
            GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(result)
            
         public:
            result(type::base);
      };
   }
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"