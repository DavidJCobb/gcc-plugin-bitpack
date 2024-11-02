#pragma once
#include <type_traits>
// GCC:
#include <gcc-plugin.h> 
#include <tree.h>

namespace gcc_helpers {
   template<typename Functor> requires std::is_invocable_v<Functor, tree>
   void for_each_in_list_tree(tree list, Functor&& functor) {
      for(; list != NULL_TREE; list = TREE_CHAIN(list)) {
         if constexpr (std::is_invocable_r_v<bool, Functor, tree>) {
            bool result = functor(list);
            if (!result)
               return;
         } else {
            functor(list);
         }
      }
   }
}