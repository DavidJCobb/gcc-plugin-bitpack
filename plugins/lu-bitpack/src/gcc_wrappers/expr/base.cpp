#include "gcc_wrappers/expr/base.h"
#include <cassert>

namespace gcc_wrappers::expr {
   bool base::suppresses_unused_warnings() {
      return TREE_USED(this->_node) != 0;
   }
   void base::suppress_unused_warnings() {
      assert(!empty());
      TREE_USED(this->_node) = 1;
   }
   void base::set_suppresses_unused_warnings(bool v) {
      assert(!empty());
      TREE_USED(this->_node) = v;
   }
   
   type base::get_result_type() const {
      if (empty())
         return {};
      type t;
      t.set_from_untyped(TREE_TYPE(this->_node));
      return t;
   }
}