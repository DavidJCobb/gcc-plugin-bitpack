#include "gcc_wrappers/expr/declare_label.h"
#include "gcc_wrappers/decl/label.h"
#include "gcc_wrappers/_node_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   GCC_NODE_WRAPPER_BOILERPLATE(declare_label)
   
   label::label(decl::label decl) {
      this->_node = build1(LABEL_EXPR, void_type_node, decl.unwrap());
   }
   
   decl::label label::declaration() const {
      return decl::label::wrap(LABEL_EXPR_LABEL(this->_node));
   }
}