#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class label;
   }
   namespace expr {
      class go_to_label : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == GOTO_EXPR;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(go_to_label)
         
         public:
            go_to_label(decl::label&);
            
            decl::label destination() const;
      };
      static_assert(sizeof(go_to_label) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"