#pragma once
#include <utility> // std:pair
#include "gcc_wrappers/node.h"
#include "gcc_wrappers/node_pointer_template.h"

namespace gcc_wrappers {
   class list {
      protected:
         node_ptr _node = NULL_TREE;
      
      public:
         class iterator {
            friend list;
            protected:
               iterator(node_ptr);
               
               node_ptr _node;
               
            public:
               std::pair<node_ptr, node_ptr> operator*(); // k, v
            
               iterator& operator++();
               iterator  operator++(int) const;
               
               bool operator==(const iterator&) const = default;
         };
         
      public:
         // Creates an empty view. It is safe to append, prepend, etc., 
         // to the empty view in order to create a new list.
         list() {}
         
         // Creates a view of an existing TREE_LIST node. Asserts that 
         // the passed-in node is of the right type.
         list(node);
         list(node_ptr);
      
         inline bool empty() const noexcept {
            return !this->_node;
         }
         
         iterator begin();
         iterator end();
         iterator at(size_t n);
         
         constexpr const tree as_raw() const {
            return this->_node.unwrap();
         }
         constexpr tree as_raw() {
            return this->_node.unwrap();
         }
         
         size_t size() const;
         
         node     front(); // first k/v pair
         node     back(); // last k/v pair
         node     nth_pair(size_t n);
         node_ptr nth_key(size_t n);
         node_ptr nth_value(size_t n);
         
         node_ptr find_value_by_key(node_ptr key);
         node_ptr find_pair_by_key(node_ptr key);
         node_ptr find_pair_by_value(node_ptr value);
         
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
         
         void append(node_ptr key, node_ptr value);
         
         void concat(list&&);
         
         // This is only safe to call on a list head. We can't actually 
         // stop you from wrapping a non-head list node using this class 
         // and then prepending to that, which is why I'm warning you here.
         //
         // Lists are singly-linked. Since they're implemented as key/value 
         // pairs, we could in theory just swap keys and values as a "safe 
         // prepend" option, but that feels slow, so I'm not gonna bother.
         void prepend(node_ptr key, node_ptr value);
   };
   DECLARE_GCC_NODE_POINTER_WRAPPER(list);
}

#include "gcc_wrappers/list.inl"