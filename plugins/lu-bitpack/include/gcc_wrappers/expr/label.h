#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class label;
   }
   namespace expr {
      class label : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == LABEL_EXPR;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(label)
         
         public:
            label(decl::label);
            
            decl::label declaration() const;
      };
      static_assert(sizeof(label) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"