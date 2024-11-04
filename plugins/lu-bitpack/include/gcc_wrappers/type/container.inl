#pragma once
#include <type_traits>
#include "gcc_wrappers/type/container.h"

namespace gcc_wrappers::type {
   template<typename Functor>
   void container::for_each_field(Functor&& functor) const {
      for(auto item : this->all_members()) {
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
   void container::for_each_referenceable_field(Functor&& functor) const {
      for(auto item : this->all_members()) {
         if (TREE_CODE(item) != FIELD_DECL)
            continue;
         
         if (DECL_NAME(item) == NULL_TREE) {
            //
            // This data member is unnamed. If it's a `container` itself, then 
            // recurse over its fields.
            //
            auto type_node = TREE_TYPE(item);
            if (container::node_is(type_node))
               container::from_untyped(type_node).for_each_referenceable_field(functor);
            continue;
         }
         
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
      for(auto item : this->all_members()) {
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