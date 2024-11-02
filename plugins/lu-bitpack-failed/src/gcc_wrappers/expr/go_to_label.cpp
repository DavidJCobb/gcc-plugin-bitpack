#include "gcc_wrappers/expr/go_to_label.h"
#include "gcc_wrappers/decl/label.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(go_to_label)
   
   go_to_label::go_to_label(
      decl::label& decl
   ) {
      this->_node = build1(GOTO_EXPR, void_type_node, decl.as_untyped());
   }
}