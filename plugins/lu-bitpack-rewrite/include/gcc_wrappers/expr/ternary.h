#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      class ternary : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == COND_EXPR;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(ternary)
         
         public:
            ternary(
               type::base result_type,
               value      condition,
               expr::base if_true,
               expr::base if_false
            );
            
            base get_condition();
            base get_true_branch();
            base get_false_branch();
            
            void set_true_branch(expr::base);
            void set_false_branch(expr::base);
      };
      static_assert(sizeof(ternary) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"