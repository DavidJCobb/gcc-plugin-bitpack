#include "gcc_wrappers/expr/return_result.h"
#include "gcc_wrappers/decl/result.h"
#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/_node_ref_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(return_result)
   
   return_result::return_result(const decl::result& res) {
      this->_node = build1(
         RETURN_EXPR,
         res.value_type().as_raw(),
         res.as_raw()
      );
   }
   return_result::return_result(const expr::assign& expr) {
      assert(decl::result::raw_node_is(expr.dst().as_raw()));
      this->_node = build1(
         RETURN_EXPR,
         expr.value_type().as_raw(),
         expr.as_raw()
      );
   }
}