#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers::expr {
   class ternary : public base {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == COND_EXPR;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(ternary)
      
      public:
         ternary(
            type::base     result_type,
            value          condition,
            expr::base_ptr if_true = nullptr,
            expr::base_ptr if_false = nullptr
         );
         
         base_ptr get_condition();
         base_ptr get_true_branch();
         base_ptr get_false_branch();
         
         void set_true_branch(expr::base);
         void set_false_branch(expr::base);
   };
   DECLARE_GCC_NODE_POINTER_WRAPPER(ternary);
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"