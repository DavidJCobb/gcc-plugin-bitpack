#pragma once
#include "gcc_wrappers/chain_node.h"

namespace gcc_wrappers {
   template<typename Functor>
   void chain_node::for_each(Functor&& functor) {
      for(auto item = this->_node; item != NULL_TREE; item = TREE_CHAIN(item)) {
         if constexpr (std::is_invocable_r_v<bool, Functor, tree>) {
            if (!functor(item))
               return;
         } else {
            functor(item);
         }
      }
   }
}