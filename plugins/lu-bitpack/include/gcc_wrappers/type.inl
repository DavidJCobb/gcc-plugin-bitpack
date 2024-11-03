#pragma once
#include <cassert>
#include "gcc_wrappers/type.h"
// GCC:
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_wrappers {
   //#pragma region Enums
      template<typename Functor>
      void type::for_each_enum_member(Functor&& functor) const {
         assert(is_enum());
         for(auto item = TYPE_VALUES(_node); item != NULL_TREE; item = TREE_CHAIN(item)) {
            const auto member = enum_member{
               .name  = std::string_view(IDENTIFIER_POINTER(TREE_PURPOSE(item))),
               .value = (intmax_t)TREE_INT_CST_LOW(TREE_VALUE(item)),
            };
            if constexpr (std::is_invocable_r_v<bool, Functor, const enum_member&>) {
               if (!functor(member))
                  break;
            } else {
               functor(member);
            }
         }
      }
   //#pragma endregion
   
   //#pragma region Functions
      template<typename Functor>
      void type::for_each_function_argument_type(Functor&& functor) const {
         assert(is_function());
         auto args = list_node(TYPE_ARG_TYPES(this->_node));
         args.for_each_value(functor);
      }
   //#pragma endregion
   
   //#pragma region Structs and unions
      template<typename Functor>
      void type::for_each_field(Functor&& functor) const {
         if (!is_record() && !is_union())
            return;
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
      void type::for_each_static_data_member(Functor&& functor) const {
         if (!is_record() && !is_union())
            return;
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
   //#pragma endregion
}