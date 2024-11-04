#pragma once
#include <type_traits>
#include "gcc_wrappers/type/container.h"

namespace gcc_wrappers::type {
   template<typename Functor>
   void container::for_each_field(Functor&& functor) const {
      for(auto item = TYPE_FIELDS(_node); item != NULL_TREE; item = TREE_CHAIN(item)) {
         if (TREE_CODE(item) != FIELD_DECL)
            continue;
         if constexpr (std::is_invocable_r_v<bool, Functor, tree>) {
            if (!functor(item))
               break;
         } else {
            functor(item);
         }
      }
   }
   
   template<typename Functor>
   void container::for_each_static_data_member(Functor&& functor) const {
      for(auto item = TYPE_FIELDS(_node); item != NULL_TREE; item = TREE_CHAIN(item)) {
         if (TREE_CODE(item) != VAR_DECL)
            continue;
         if constexpr (std::is_invocable_r_v<bool, Functor, tree>) {
            if (!functor(item))
               break;
         } else {
            functor(item);
         }
      }
   }
}