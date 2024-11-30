#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class result;
   }
   namespace expr {
      class assign;
   }
}

namespace gcc_wrappers::expr {
   class return_result : public base {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == MODIFY_EXPR;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(return_result)
      
      public:
         return_result(const decl::result&);
         
         // The `dst` must be a `decl::result`.
         return_result(const expr::assign&); // MODIFY_EXPR
         
         // TODO: allow INIT_EXPR
   };
   DECLARE_GCC_NODE_POINTER_WRAPPER(return_result);

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"