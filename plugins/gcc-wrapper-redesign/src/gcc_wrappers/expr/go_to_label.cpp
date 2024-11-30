#include "gcc_wrappers/expr/go_to_label.h"
#include "gcc_wrappers/decl/label.h"
#include "gcc_wrappers/_node_ref_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(go_to_label)
   
   go_to_label::go_to_label(decl::label& decl) {
      this->_node = build1(GOTO_EXPR, void_type_node, decl.as_raw());
   }
   
   decl::label go_to_label::destination() const {
      return decl::label::wrap(GOTO_DESTINATION(this->_node));
   }
}