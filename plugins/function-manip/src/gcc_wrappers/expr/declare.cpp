#include "gcc_wrappers/expr/declare.h"
#include "gcc_wrappers/decl/variable.h"

namespace gcc_wrappers::decl {
   /*static*/ bool declare::tree_is(tree node) {
      return TREE_CODE(node) == DECLARE_EXPR;
   }
   
   void declare::set_from_untyped(tree src) {
      if (src != NULL_TREE) {
         assert(tree_is(src) && "expr::declare::set_from_untyped must be given a DECLARE_EXPR");
      }
      this->_node = src;
   }
   
   /*static*/ declare declare::create(
      decl::variable& decl,
      location_t      source_location
   ) {
      variable out;
      out._node = build_stmt(source_location, DECL_EXPR, decl.as_untyped());
      return out;
   }
}