#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(call)
   
   decl::function call::callee() const {
      return decl::function::from_untyped(
         get_callee_fndecl(this->_node) // tree.h
      );
   }
}