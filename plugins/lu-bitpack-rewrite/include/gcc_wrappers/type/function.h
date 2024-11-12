#pragma once
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::type {
   class function : public base {
      public:
         static bool node_is(tree t) {
            return t != NULL_TREE && TREE_CODE(t) == FUNCTION_TYPE;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(function)
         
      public:
         base return_type() const;
         
         // Returns a list_node of raw arguments; the values are the argument types.
         // If this is not a varargs function, then the last element is the void type.
         list_node arguments() const;
         
         base nth_argument_type(size_t n) const;
         
         bool is_varargs() const;
         
         // Does not include varargs; does include `this` for member functions.
         size_t fixed_argument_count() const;
         
         // weird C-only jank: `void f()` takes a variable number of arguments
         // (but isn't varargs??)
         bool is_unprototyped() const;
         
         template<typename Functor>
         void for_each_argument_type(Functor&& functor) const;
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"
#include "gcc_wrappers/type/function.inl"