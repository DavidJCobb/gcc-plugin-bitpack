#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(integer_constant)
   
   integer_constant::integer_constant(type t, int n) {
      this->_node = build_int_cst(t.as_untyped(), n);
   }
}