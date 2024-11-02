#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class result;
   }
   namespace expr {
      class assign;
   }
}

namespace gcc_wrappers {
   namespace expr {
      class return_result : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == MODIFY_EXPR;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(return_result)
         
         public:
            return_result(const decl::result&);
            
            // The `dst` must be a `decl::result`.
            return_result(const expr::assign&); // MODIFY_EXPR
            
            // TODO: allow INIT_EXPR
      };
      static_assert(sizeof(return_result) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"