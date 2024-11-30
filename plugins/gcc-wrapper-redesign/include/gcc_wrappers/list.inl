#pragma once
#include "gcc_wrappers/list.h"

namespace gcc_wrappers {
   // run the functor for each item's TREE_PURPOSE
   template<typename Functor>
   void list::for_each_key(Functor&& functor) {
      if (!this->_node)
         return;
      for(auto raw = this->_node->as_raw(); raw != NULL_TREE; raw = TREE_CHAIN(raw)) {
         auto key = node_ptr(TREE_PURPOSE(raw));
         if constexpr (std::is_invocable_r_v<bool, Functor, node_ptr>) {
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
      if (!this->_node)
         return;
      for(auto raw = this->_node->as_raw(); raw != NULL_TREE; raw = TREE_CHAIN(raw)) {
         auto value = node_ptr(TREE_VALUE(raw));
         if constexpr (std::is_invocable_r_v<bool, Functor, node_ptr>) {
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
      if (!this->_node)
         return;
      for(auto raw = this->_node->as_raw(); raw != NULL_TREE; raw = TREE_CHAIN(raw)) {
         auto key   = node_ptr(TREE_PURPOSE(raw));
         auto value = node_ptr(TREE_VALUE(raw));
         if constexpr (std::is_invocable_r_v<bool, Functor, node_ptr, node_ptr>) {
            if (!functor(key, value))
               return;
         } else {
            functor(key, value);
         }
      }
   }
}