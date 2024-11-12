#pragma once
#include <type_traits>
#include "gcc_wrappers/type/function.h"

namespace gcc_wrappers::type {
   template<typename Functor>
   void function::for_each_argument_type(Functor&& functor) const {
      auto args = list_node(TYPE_ARG_TYPES(this->_node));
      args.for_each_value(functor);
   }
}