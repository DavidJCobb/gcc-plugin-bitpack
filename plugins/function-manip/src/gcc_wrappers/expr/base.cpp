#include "gcc_wrappers/expr/base.h"
#include <cassert>

#include <c-family/c-common.h> // c_build_qualified_type; various predefined type nodes

namespace gcc_wrappers::expr {
   /*static*/ bool base::tree_is(tree node) {
      return EXPR_P(node);
   }
   
   void base::suppress_unused_warnings() {
      assert(!empty());
      TREE_USED(this->_node) = 1;
   }
   
   void base::set_from_untyped(tree src) {
      if (src != NULL_TREE) {
         assert(tree_is(src) && "expr::base::set_from_untyped must be given an expr");
      }
      this->_node = src;
   }
}