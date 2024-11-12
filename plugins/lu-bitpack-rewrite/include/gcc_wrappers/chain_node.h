#pragma once
#include "gcc_wrappers/_wrapped_tree_node.h"

namespace gcc_wrappers {
   class chain_node {
      protected:
         tree _node = NULL_TREE;
         
      public:
         class iterator {
            friend chain_node;
            protected:
               iterator(tree);
               
               tree _node = NULL_TREE;
               
            public:
               tree operator*();
            
               iterator& operator++();
               iterator  operator++(int) const;
               
               bool operator==(const iterator&) const = default;
         };
         
      public:
         chain_node() {}
         chain_node(tree t) : _node(t) {}
      
         inline bool empty() const {
            return this->_node == NULL_TREE;
         }
         
         iterator begin();
         iterator end();
         iterator at(size_t n);
         
         constexpr const tree& as_untyped() const {
            return this->_node;
         }
         constexpr tree& as_untyped() {
            return this->_node;
         }
         
         size_t size() const;
         
         tree untyped_back();
         tree untyped_nth(size_t n);
         
         template<typename Functor>
         void for_each(Functor&&);
         
         bool contains_untyped(tree) const;
         
         //
         // Functions for mutating a chain:
         //
         
         void concat(chain_node);
         void reverse();
   };
}

#include "gcc_wrappers/chain_node.inl"