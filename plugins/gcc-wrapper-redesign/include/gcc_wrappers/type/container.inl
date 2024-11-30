#pragma once
#include <type_traits>
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/decl/variable.h"

namespace gcc_wrappers::type {
   template<typename Functor>
   void container::for_each_field(Functor&& functor) const {
      for(auto item : this->member_chain()) {
         if (TREE_CODE(item) != FIELD_DECL)
            continue;
         auto decl = decl::field::wrap(item);
         if constexpr (std::is_invocable_r_v<bool, Functor, decl::field>) {
            if (!functor(decl))
               break;
         } else {
            functor(decl);
         }
      }
   }
   
   template<typename Functor>
   void container::for_each_referenceable_field(Functor&& functor) const {
      for(auto item : this->member_chain()) {
         if (TREE_CODE(item) != FIELD_DECL)
            continue;
         
         if (DECL_VIRTUAL_P(item))
            //
            // This is the VTBL pointer. Skip it.
            //
            continue;
         
         if (DECL_NAME(item) == NULL_TREE) {
            //
            // This data member is unnamed. If it's a `container` itself, then 
            // recurse over its fields.
            //
            auto type_node = TREE_TYPE(item);
            if (container::raw_node_is(type_node))
               container::wrap(type_node).for_each_referenceable_field(functor);
            continue;
         }
         
         auto decl = decl::field::wrap(item);
         if constexpr (std::is_invocable_r_v<bool, Functor, decl::field>) {
            if (!functor(decl))
               break;
         } else {
            functor(decl);
         }
      }
   }
   
   template<typename Functor>
   void container::for_each_static_data_member(Functor&& functor) const {
      for(auto item : this->member_chain()) {
         if (TREE_CODE(item) != VAR_DECL)
            continue;
         auto decl = decl::variable::wrap(item);
         if constexpr (std::is_invocable_r_v<bool, Functor, decl::variable>) {
            if (!functor(decl))
               break;
         } else {
            functor(decl);
         }
      }
   }
}