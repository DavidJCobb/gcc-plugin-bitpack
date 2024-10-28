#pragma once
#include <cassert>
#include <concepts>
#include <type_traits>
// GCC:
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_wrappers {
   namespace impl {
      template<typename Wrapper>
      concept has_typecheck = requires (tree t) {
         { Wrapper::node_is(t) } -> std::same_as<bool>;
      };
   }
   
   class _wrapped_tree_node {
      protected:
         enum class _from_untyped_tag {};
         
         template<typename Self>
         void _set_from_untyped(tree t) {
            if constexpr (impl::has_typecheck<Self>) {
               if (t != NULL_TREE) {
                  assert(Self::node_is(t));
               }
            }
            this->_node = t;
         }
         
         template<typename Self>
         static Self _from_unwrapped(tree t) {
            static_assert(sizeof(Self) == sizeof(_wrapped_tree_node));
            if constexpr (impl::has_typecheck<Self>) {
               if (t != NULL_TREE) {
                  assert(Self::node_is(t));
               }
            }
            _wrapped_tree_node wrap;
            wrap._node = t;
            return *(Self*)&wrap;
         }
      
      protected:
         tree _node = NULL_TREE;
      
      public:
         inline bool empty() const {
            return this->_node == NULL_TREE;
         }
         
         int code() const {
            return TREE_CODE(this->_node); // TODO: do we need to null check here?
         }
         
         constexpr const tree& as_untyped() const {
            return this->_node;
         }
         constexpr tree& as_untyped() {
            return this->_node;
         }
         
         inline void set_from_untyped(tree t) {
            this->_node = t;
         }
   };
}