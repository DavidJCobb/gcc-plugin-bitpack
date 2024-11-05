#pragma once
#include <utility> // std:pair
#include "gcc_wrappers/_wrapped_tree_node.h"

namespace gcc_wrappers {
   class list_node {
      protected:
         tree _node = NULL_TREE;
         
      public:
         class iterator {
            friend list_node;
            protected:
               iterator(tree);
               
               tree _node = NULL_TREE;
               
            public:
               std::pair<tree, tree> operator*(); // k, v
            
               iterator& operator++();
               iterator  operator++(int) const;
               
               bool operator==(const iterator&) const = default;
         };
         
      public:
         list_node() {}
         list_node(tree t) : _node(t) {}
      
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
         
         tree untyped_back(); // last k/v pair
         tree untyped_nth_kv_pair(size_t n);
         tree untyped_nth_key(size_t n);
         tree untyped_nth_value(size_t n);
         
         // Returns NULL_TREE on failure or if it's the actual value.
         tree untyped_value_by_untyped_key(tree);
         
         tree untyped_kv_pair_for_untyped_value(tree);
         
         // run the functor for each item's TREE_PURPOSE
         template<typename Functor>
         void for_each_key(Functor&&);
         
         // run the functor for each item's TREE_VALUE
         template<typename Functor>
         void for_each_value(Functor&&);
         
         // run the functor for each item's TREE_PURPOSE and TREE_VALUE
         template<typename Functor>
         void for_each_kv_pair(Functor&&);
         
         //
         // Functions for mutating a list:
         //
         
         void concat(list_node);
         
         // Only call this on the list head. Sets `this->_node` to the 
         // newly-created pair node.
         void insert_at_head(tree key, tree value);
         
         // assert(!empty());
         void insert_after(tree key, tree value);
   };
}

#include "gcc_wrappers/list_node.inl"