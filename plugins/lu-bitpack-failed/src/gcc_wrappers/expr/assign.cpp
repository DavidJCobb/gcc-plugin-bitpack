#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"
#include <c-family/c-common.h>

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(assign)
   
   assign::assign(const value& dst, const value& src) {
      auto dst_type = dst.value_type();
      
      // Do some of GCC's own validation ahead of time
      assert(dst_type.is_complete());
      assert(!dst_type.is_array());
      assert(dst.is_lvalue());
      
      // build_modify_expr declared in c-common.h, defined in c/c-typeck.cc
      this->_node = build_modify_expr(
         UNKNOWN_LOCATION, // expression source location
         dst.as_untyped(), // LHS
         NULL_TREE,        // original type of LHS (needed if it's an enum bitfield)
         NOP_EXPR,         // operation (e.g. PLUS_EXPR if we want to make a +=)
         UNKNOWN_LOCATION, // RHS source location
         src.as_untyped(), // RHS
         NULL_TREE         // original type of RHS (needed if it's an enum)
      );
      /*this->_node = build2(
         MODIFY_EXPR,
         dst.value_type().as_untyped(),
         dst.as_untyped(),
         src.as_untyped()
      );*/
   }
   
   /*static*/ assign assign::make_with_modification(
      const value& dst,
      tree_code    operation,
      const value& src
   ) {
      return assign::from_untyped(
         build_modify_expr(
            UNKNOWN_LOCATION, // expression source location
            dst.as_untyped(), // LHS
            NULL_TREE,        // original type of LHS (needed if it's an enum bitfield)
            operation,        // operation (e.g. PLUS_EXPR if we want to make a +=)
            UNKNOWN_LOCATION, // RHS source location
            src.as_untyped(), // RHS
            NULL_TREE         // original type of RHS (needed if it's an enum)
         )
      );
   }
   
   value assign::src() const {
      value v;
      v.set_from_untyped(TREE_OPERAND(this->_node, 0));
      return v;
   }
   value assign::dst() const {
      value v;
      v.set_from_untyped(TREE_OPERAND(this->_node, 1));
      return v;
   }
}