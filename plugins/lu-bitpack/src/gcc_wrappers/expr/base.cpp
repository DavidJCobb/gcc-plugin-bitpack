#include "gcc_wrappers/expr/base.h"
#include <cassert>
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(base)
   
   location_t base::source_location() const {
      if (empty())
         return UNKNOWN_LOCATION;
      return EXPR_LOCATION(this->_node);
   }
   void base::set_source_location(location_t loc, bool wrap_if_necessary) {
      assert(!empty());
      if (!CAN_HAVE_LOCATION_P(this->_node)) {
         this->_node = maybe_wrap_with_location(this->_node, loc);
         return;
      }
      protected_set_expr_location(this->_node, loc);
   }
   
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
   
   type::base base::get_result_type() const {
      if (empty())
         return {};
      return type::base::from_untyped(TREE_TYPE(this->_node));
   }
}