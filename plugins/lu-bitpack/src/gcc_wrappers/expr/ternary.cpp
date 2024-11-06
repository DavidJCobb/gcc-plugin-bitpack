#include "gcc_wrappers/expr/ternary.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(ternary)
   
   ternary::ternary(
      type::base result_type,
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
   
   base ternary::get_condition() {
      assert(!empty());
      return base::from_untyped(COND_EXPR_COND(this->_node));
   }
   base ternary::get_true_branch() {
      assert(!empty());
      return base::from_untyped(COND_EXPR_THEN(this->_node));
   }
   base ternary::get_false_branch() {
      assert(!empty());
      return base::from_untyped(COND_EXPR_ELSE(this->_node));
   }
   
   void ternary::set_true_branch(expr::base e) {
      assert(!empty());
      COND_EXPR_THEN(this->_node) = e.as_untyped();
   }
   void ternary::set_false_branch(expr::base e) {
      assert(!empty());
      COND_EXPR_ELSE(this->_node) = e.as_untyped();
   }
}