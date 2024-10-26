#pragma once
#include <gcc-plugin.h>
#include <tree.h>
#include "gcc_wrappers/type.h"

namespace gcc_wrappers::decl {
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
         
         void mark_artificial(); // mark the decl as compiler-generated
         void mark_used();
         
         type get_value_type() const;
         
         void set_from_untyped(tree);
   };
}