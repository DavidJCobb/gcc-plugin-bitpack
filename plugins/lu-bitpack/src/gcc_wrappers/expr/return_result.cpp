#include "gcc_wrappers/expr/return_result.h"
#include "gcc_wrappers/decl/result.h"
#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(return_result)
   
   return_result::return_result(const decl::result& res) {
      this->_node = build1(
         RETURN_EXPR,
         res.value_type().as_untyped(),
         res.as_untyped()
      );
   }
   return_result::return_result(const expr::assign& expr) {
      assert(decl::result::node_is(expr.dst().as_untyped()));
      this->_node = build1(
         RETURN_EXPR,
         expr.value_type().as_untyped(),
         expr.as_untyped()
      );
   }
}