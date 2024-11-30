#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/_node_ref_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(call)
   
   decl::function call::callee() const {
      return decl::function::from_untyped(
         get_callee_fndecl(this->_node) // tree.h
      );
   }
   
   size_t call::argument_count() const {
      if (empty())
         return 0;
      return call_expr_nargs(this->_node);
   }
   value call::nth_argument(size_t n) const {
      assert(!empty());
      assert(n < argument_count());
      value v;
      v.set_from_untyped(CALL_EXPR_ARG(this->_node, n));
      return v;
   }
}