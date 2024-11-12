#include "gcc_wrappers/expr/base.h"
#include <cassert>
#include <c-family/c-common.h> // c_fully_fold
#include <fold-const.h> // fold
#include "gcc_headers/plugin-version.h"
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
   
   base base::folded(bool manifestly_constant_evaluated) const {
      assert(!empty());
      if (manifestly_constant_evaluated) {
         #if GCCPLUGIN_VERSION_MAJOR >= 12
            return ::fold_init(this->_node);
         #else
            auto node = fold_build1_initializer_loc(UNKNOWN_LOCATION, NOP_EXPR, TREE_TYPE(this->_node), this->_node);
            //
            // It's safe to abandon `node`; GCC uses garbage collection, 
            // so this shouldn't leak (for long).
            //
            auto op = TREE_OPERAND(node, 0);
            TREE_OPERAND(node, 0) = NULL_TREE;
            return from_untyped(op);
         #endif
      }
      return from_untyped(::fold(this->_node));
   }
   
   base base::fold_to_c_constant(
      bool initializer,
      bool as_lvalue
   ) const {
      assert(!empty());
      
      bool maybe_const = true;
      auto expr = ::c_fully_fold(this->_node, initializer, &maybe_const, as_lvalue);
      if (!maybe_const)
         return {};
      return base::from_untyped(expr);
   }
   
   bool base::is_associative_operator() const {
      if (empty())
         return false;
      return associative_tree_code(TREE_CODE(this->_node));
   }
   bool base::is_commutative_operator() const {
      if (empty())
         return false;
      return commutative_tree_code(TREE_CODE(this->_node));
   }
   size_t base::operator_operand_count() const {
      if (empty())
         return 0;
      return TREE_CODE_LENGTH(TREE_CODE(this->_node));
   }
}