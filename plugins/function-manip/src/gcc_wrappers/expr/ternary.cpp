#include "gcc_wrappers/expr/ternary.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(ternary)
   
   ternary::ternary(
      type       result_type,
      value      condition,
      expr::base if_true,
      expr::base if_false
   ) {
      this->_node = build3(
         COND_EXPR,
         result_type.as_untyped(),
         condition.as_untyped(),
         if_true.as_untyped(),
         if_false.as_untyped()
      );
   }
}