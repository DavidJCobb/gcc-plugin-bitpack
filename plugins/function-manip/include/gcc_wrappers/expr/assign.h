#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      class assign : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == MODIFY_EXPR;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(assign)
         
         public:
            assign(const value& dst, const value& src);
            
            // Creates nested expressions for e.g. +=
            static assign make_with_modification(
               const value& dst,
               tree_code    operation,
               const value& src
            );
            
            value src() const;
            value dst() const;
      };
      static_assert(sizeof(assign) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"