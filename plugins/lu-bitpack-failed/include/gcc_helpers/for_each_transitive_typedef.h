#pragma once
#include <type_traits>
// GCC:
#include <gcc-plugin.h> 
#include <tree.h>

#include "gcc_helpers/extract_string_constant_from_tree.h"
#include "gcc_helpers/for_each_in_list_tree.h"

namespace gcc_helpers {
   template<typename Functor> requires std::is_invocable_v<Functor, tree>
   extern void for_each_transitive_typedef(tree type, Functor&& functor) {
      if (!TYPE_P(type))
         return;
      
      auto _is_typedef = [](tree decl) -> bool {
         return decl != NULL_TREE &&
                TREE_CODE(decl) == TYPE_DECL &&
                DECL_ORIGINAL_TYPE(decl) != NULL_TREE;
      };
      
      if constexpr (std::is_invocable_r_v<bool, Functor, tree>) {
         bool result = functor(type);
         if (!result)
            return;
      } else {
         functor(type);
      }
      
      auto decl = TYPE_NAME(type);
      while (_is_typedef(decl)) {
         auto orig = DECL_ORIGINAL_TYPE(decl);
         if (orig == NULL_TREE || !TYPE_P(orig))
            break;
         
         if constexpr (std::is_invocable_r_v<bool, Functor, tree>) {
            bool result = functor(orig);
            if (!result)
               return;
         } else {
            functor(orig);
         }
         
         decl = TYPE_NAME(orig);
      }
   }
}