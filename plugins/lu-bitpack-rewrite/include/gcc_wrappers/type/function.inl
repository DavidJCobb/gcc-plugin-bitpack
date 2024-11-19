#pragma once
#include <type_traits>
#include "gcc_wrappers/type/function.h"

namespace gcc_wrappers::type {
   template<typename Functor>
   void function::for_each_argument_type(Functor&& functor) const {
      function_args_iterator it;
      tree   raw;
      size_t i = 0;
      FOREACH_FUNCTION_ARGS(this->_node, raw, it) {
         if (raw == void_type_node || raw == error_mark_node)
            break;
         auto wrap = base::from_untyped(raw);
         if constexpr (std::is_invocable_v<Functor, base, size_t>) {
            functor(wrap, i++);
         } else {
            functor(wrap);
         }
      }
   }
}