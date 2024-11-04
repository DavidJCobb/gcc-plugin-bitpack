#pragma once
#include <type_traits>
#include "gcc_wrappers/type/function.h"

namespace gcc_wrappers::type {
   template<typename... Args> requires (std::is_base_of_v<base, Args> && ...)
   function make_function_type(base return_type, Args... args) {
      return function::from_untyped(build_function_type_list( // tree.h
         return_type.as_untyped(),
         args.as_untyped()...,
         NULL_TREE
      ));
   };
}