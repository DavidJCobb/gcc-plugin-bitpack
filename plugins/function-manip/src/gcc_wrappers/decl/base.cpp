#include "gcc_wrappers/decl/base.h"
#include <cassert>

#include <c-family/c-common.h> // c_build_qualified_type; various predefined type nodes

namespace gcc_wrappers::decl {
   /*static*/ bool base::tree_is(tree node) {
      return DECL_P(node);
   }
   
   void base::mark_artificial() {
      assert(!empty());
      TREE_ARTIFICIAL(this->_node) = 1;
   }
   void base::mark_used() {
      assert(!empty());
      TREE_USED(this->_node) = 1;
   }
   
   type base::get_value_type() const {
      if (empty())
         return type{};
      
      type t;
      t.set_from_untyped(TREE_TYPE(this->_node));
      return t;
   }
   
   void base::set_from_untyped(tree src) {
      if (src != NULL_TREE) {
         assert(tree_is(src) && "decl::base::set_from_untyped must be given a decl");
      }
      this->_node = src;
   }
}