#pragma once
#include "gcc_wrappers/list_node.h"

namespace gcc_wrappers {
   // run the functor for each item's TREE_PURPOSE
   template<typename Functor>
   void list_node::for_each_key(Functor&& functor) {
      for(auto item = this->_node; item != NULL_TREE; item = TREE_CHAIN(item)) {
         auto key = TREE_PURPOSE(item);
         if constexpr (std::is_invocable_r_v<bool, Functor, tree>) {
            if (!functor(key))
               return;
         } else {
            functor(key);
         }
      }
   }
   
   // run the functor for each item's TREE_VALUE
   template<typename Functor>
   void list_node::for_each_value(Functor&& functor) {
      for(auto item = this->_node; item != NULL_TREE; item = TREE_CHAIN(item)) {
         auto value = TREE_VALUE(item);
         if constexpr (std::is_invocable_r_v<bool, Functor, tree>) {
            if (!functor(value))
               return;
         } else {
            functor(value);
         }
      }
   }
   
   // run the functor for each item's TREE_PURPOSE and TREE_VALUE
   template<typename Functor>
   void list_node::for_each_kv_pair(Functor&& functor) {
      for(auto item = this->_node; item != NULL_TREE; item = TREE_CHAIN(item)) {
         auto key   = TREE_PURPOSE(item);
         auto value = TREE_VALUE(item);
         if constexpr (std::is_invocable_r_v<bool, Functor, tree, tree>) {
            if (!functor(key, value))
               return;
         } else {
            functor(key, value);
         }
      }
   }
}