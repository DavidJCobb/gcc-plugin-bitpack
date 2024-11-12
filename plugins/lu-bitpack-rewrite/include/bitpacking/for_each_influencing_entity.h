#pragma once
#include <type_traits>
#include <vector>
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/param.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/array.h"

namespace bitpacking {
   template<typename Functor> requires std::is_invocable_v<Functor, tree>
   bool for_each_influencing_entity(gcc_wrappers::type::base type, Functor&& functor) {
      if (type.is_array()) {
         if (!for_each_influencing_entity(type.as_array().value_type(), functor))
            return false;
      }
      auto decl = type.declaration();
      if (decl.empty()) {
         if constexpr (std::is_invocable_r_v<Functor, bool, tree>) {
            return functor(type.as_untyped());
         } else {
            functor(type.as_untyped());
         }
         return true;
      }
      //
      // Handle transitive typedefs. For example, given `typedef a b`, we want 
      // to apply bitpacking options for `a` when we see `b`, before we apply 
      // any bitpacking options for `b`.
      //
      std::vector<gcc_wrappers::type::base> transitives;
      transitives.push_back(type);
      do {
         auto tran = decl.is_synonym_of();
         if (tran.empty())
            break;
         transitives.push_back(tran);
         decl = tran.declaration();
      } while (!decl.empty());
      for(auto it = transitives.rbegin(); it != transitives.rend(); ++it) {
         auto tran = *it;
         auto decl = tran.declaration();
         if constexpr (std::is_invocable_r_v<Functor, bool, tree>) {
            if (!functor(tran.as_untyped()))
               return false;
         } else {
            functor(tran.as_untyped());
         }
         if (!decl.empty()) {
            if constexpr (std::is_invocable_r_v<Functor, bool, tree>) {
               if (!functor(decl.as_untyped()))
                  return false;
            } else {
               functor(decl.as_untyped());
            }
         }
      }
      return true;
   }
   
   template<typename Functor, typename NodeWrapper> requires (
      std::is_invocable_v<Functor, tree> &&
      std::is_same_v<NodeWrapper, gcc_wrappers::decl::field> ||
      std::is_same_v<NodeWrapper, gcc_wrappers::decl::param> ||
      std::is_same_v<NodeWrapper, gcc_wrappers::decl::variable>
   )
   void for_each_influencing_entity(NodeWrapper decl, Functor&& functor) {
      if constexpr (std::is_invocable_r_v<Functor, bool, tree>) {
         if (!functor(decl.as_untyped()))
            return;
      } else {
         functor(decl.as_untyped());
      }
      for_each_influencing_entity(decl.value_type(), functor);
   }
   
   template<typename Functor> requires std::is_invocable_v<Functor, tree>
   void for_each_influencing_entity(gcc_wrappers::decl::type_def decl, Functor&& functor) {
      for_each_influencing_entity(decl.declared(), functor);
   }
}