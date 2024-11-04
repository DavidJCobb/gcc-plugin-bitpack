#include "gcc_wrappers/decl/label.h"
#include <c-family/c-common.h> // build_unary_op
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(label)
   
   label::label(
      location_t source_location
   ) {
      this->_node = build_decl(
         source_location,
         LABEL_DECL,
         NULL_TREE, // can be an IDENTIFIER_NODE?
         void_type_node
      );
   }
   
   expr::label label::make_label_expr() {
      return expr::label(*this);
   }
   
   value label::address_of() {
      value out;
      out.set_from_untyped(build_unary_op(
         UNKNOWN_LOCATION,
         ADDR_EXPR,
         this->_node,
         false
      ));
      return out;
   }
}