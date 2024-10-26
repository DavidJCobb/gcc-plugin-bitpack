#pragma once
#include <type_traits>
// GCC plug-ins:
#include <gcc-plugin.h>
#include <tree.h>

#include "lu/tuples/for_each_type.h"
#include "lu/type_traits/function_traits.h"
#include "lu/type_traits/name_of_type.h"
#include "gcc_helpers/type_name.h"

namespace gcc_helpers {
   template<typename T>
   bool type_node_matches_template_param(tree type) {
      if (type == NULL_TREE)
         return false;
      if (TYPE_READONLY(type) != std::is_const_v<T>)
         return false;
      
      if (std::is_pointer_v<T>) {
         if (TREE_CODE(type) != POINTER_TYPE)
            return false;
         
         using target_type = std::remove_pointer_t<T>;
         return type_node_matches_template_param<target_type>(TYPE_CANONICAL(TREE_TYPE(type)));
      } else {
         if (TREE_CODE(type) == POINTER_TYPE)
            return false;
      }
      
      using bare_type = std::decay_t<T>;
      if constexpr (std::is_same_v<bare_type, void>) {
         return type == void_type_node;
      }
      
      return lu::name_of_type<bare_type>() == type_name(type);
   }

   template<typename FunctionPtrType>
   bool function_decl_matches_template_param(tree decl) {
      if (decl == NULL_TREE)
         return false; // missing
      if (TREE_CODE(decl) != FUNCTION_DECL)
         return false; // not a function
      
      using fn_traits = lu::function_traits<FunctionPtrType>;
      {
         auto fn_type     = TREE_TYPE(decl);
         if (TREE_CODE(fn_type) != FUNCTION_TYPE)
            return false; // unable to get return type?
         auto return_type = TREE_TYPE(fn_type);
         if constexpr (std::is_same_v<typename fn_traits::return_type, void>) {
            if (return_type != void_type_node)
               return false; // wrong return type (void expected)
         } else {
            if (!type_node_matches_template_param<typename fn_traits::return_type>(return_type))
               return false; // wrong return type
         }
      }
      
      size_t i     = 0;
      auto   args  = DECL_ARGUMENTS(decl);
      bool   valid = true;
      lu::tuples::for_each_type<typename fn_traits::arg_tuple>([&valid, &i, &args]<typename Arg>() {
         if (!valid)
            return;
         
         if (args == NULL_TREE) {
            valid = false; // missing argument
            return;
         }
         
         auto arg_type = TREE_TYPE(args);
         if (arg_type == NULL_TREE) {
            valid = false; // missing argument
            return;
         }
         
         if (arg_type == void_type_node) { // indicates end of non-vargs argument list
            if constexpr (!std::is_same_v<Arg, void>) {
               valid = false; // not enough args
               return;
            }
         }
         if (!type_node_matches_template_param<Arg>(arg_type)) {
            valid = false; // type mismatch
            return;
         }
         
         args = TREE_CHAIN(args); // to next
      });
      if (!valid)
         return false; // requirements checked in the lambda were not met
      if (args != NULL_TREE)
         return false; // too many args
      
      return true;
   }
}