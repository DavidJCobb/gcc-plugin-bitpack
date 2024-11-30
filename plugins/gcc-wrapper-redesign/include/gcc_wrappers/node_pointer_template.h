#pragma once
#include <cassert>
#include "gcc_wrappers/node.h"
// GCC:
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_wrappers {
   template<typename RefWrapper>
   class node_pointer_template {
      public:
         using reference = RefWrapper;
      
      protected:
         tree _node = NULL_TREE;
         
         void _require_not_empty() const {
            assert(this->_node != NULL_TREE);
         }
         
      public:
         constexpr node_pointer_template() {}
         constexpr node_pointer_template(std::nullptr_t) {}
         
         // Copy-construct from pointer-wrapper.
         template<typename Subclass> requires std::is_base_of_v<reference, Subclass>
         constexpr node_pointer_template(const node_pointer_template<Subclass>& o) : _node(o._node) {}
         
         // Move-construct from pointer-wrapper.
         template<typename Subclass> requires std::is_base_of_v<reference, Subclass>
         constexpr node_pointer_template(node_pointer_template<Subclass>&& o) {
            this->_node = o._node;
            o._node = NULL_TREE;
         }
         
         // Construct from reference-wrapper. (We do not overload the 
         // address-of operator on reference-wrappers.)
         template<typename Subclass> requires std::is_base_of_v<reference, Subclass>
         constexpr node_pointer_template(const Subclass& o) {
            this->_node = o.as_raw();
         }
         
         // Construct from bare node pointer.
         node_pointer_template(tree t) {
            if constexpr (impl::has_typecheck<reference>) {
               if (t != NULL_TREE) {
                  assert(reference::raw_node_is(t));
               }
            }
            this->_node = t;
         }
         
         constexpr bool operator==(const node_pointer_template&) const noexcept = default;
         
         constexpr tree unwrap() const {
            return this->_node;
         }
      
         reference operator*() {
            _require_not_empty();
            return reference::wrap(this->_node);
         }
         const reference operator*() const {
            _require_not_empty();
            return reference::wrap(this->_node);
         }
         
         // disgusting hack
         reference* operator->() {
            _require_not_empty();
            return (reference*) this;
         }
         const reference operator->() const {
            _require_not_empty();
            return (const reference*) this;
         }
         
         constexpr operator bool() const noexcept {
            return this->_node != NULL_TREE;
         }
         constexpr bool operator!() const noexcept {
            return this->_node == NULL_TREE;
         }
   };
   
   using node_ptr = node_pointer_template<node>;
}