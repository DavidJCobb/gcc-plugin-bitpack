#pragma once
#include "gcc_wrappers/decl/base_value.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/value.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class param : public base_value {
         public:
            static bool raw_node_is(tree t) {
               return TREE_CODE(t) == PARM_DECL;
            }
            GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(param)
            
         public:
            type::base effective_type() const;
      };
   }
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"