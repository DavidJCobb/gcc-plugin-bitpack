#include "gcc_wrappers/expr/label.h"
#include "gcc_wrappers/decl/label.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(label)
   
   label::label(decl::label decl) {
      this->_node = build1(LABEL_EXPR, void_type_node, decl.as_untyped());
   }
   
   decl::label label::declaration() const {
      assert(!empty());
      return decl::label::from_untyped(LABEL_EXPR_LABEL(this->_node));
   }
}