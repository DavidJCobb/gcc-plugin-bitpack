#include "gcc_wrappers/expr/ternary.h"
#include "gcc_wrappers/_node_ref_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(ternary)
   
   ternary::ternary(
      type::base     result_type,
      value          condition,
      expr::base_ptr if_true,
      expr::base_ptr if_false
   ) {
      this->_node = build3(
         COND_EXPR,
         result_type.as_raw(),
         condition.as_raw(),
         if_true.unwrap(),
         if_false.unwrap()
      );
   }
   
   base_ptr ternary::get_condition() {
      return COND_EXPR_COND(this->_node);
   }
   base_ptr ternary::get_true_branch() {
      return COND_EXPR_THEN(this->_node);
   }
   base_ptr ternary::get_false_branch() {
      return COND_EXPR_ELSE(this->_node);
   }
   
   void ternary::set_true_branch(expr::base e) {
      COND_EXPR_THEN(this->_node) = e.as_raw();
   }
   void ternary::set_false_branch(expr::base e) {
      COND_EXPR_ELSE(this->_node) = e.as_raw();
   }
}