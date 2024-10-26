#pragma once
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_wrappers::expr {
   class base {
      protected:
         tree _node = NULL_TREE;
         
      public:
         static bool tree_is(tree);
         
         constexpr const tree& as_untyped() const {
            return this->_node;
         }
         constexpr tree& as_untyped() {
            return this->_node;
         }
         
         constexpr bool empty() const noexcept {
            return this->_node == NULL_TREE;
         }
         
         void suppress_unused_warnings();
         
         void set_from_untyped(tree);
   };
}