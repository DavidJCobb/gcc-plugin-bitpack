#pragma once
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/list.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers::type {
   class function : public base {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == FUNCTION_TYPE;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(function)
         
      public:
         template<typename... Args> requires (std::is_base_of_v<base, Args> && ...)
         function(base return_type, Args... args) {
            return wrap(build_function_type_list( // tree.h
               return_type.as_untyped(),
               args.as_untyped()...,
               NULL_TREE
            ));
         };
         
         base return_type() const;
         
         // Returns a list_node of raw arguments; the values are the argument types.
         // If this is not a varargs function, then the last element is the void type.
         list arguments() const;
         
         // We index arguments from zero. GCC likes to index them from one, at 
         // least in some places.
         base nth_argument_type(size_t n) const;
         
         bool is_varargs() const;
         
         // Does not include varargs; does include `this` for member functions.
         size_t fixed_argument_count() const;
         
         // weird C-only jank: `void f()` takes a variable number of arguments
         // (but isn't varargs??)
         bool is_unprototyped() const;
         
         // Supported functor signatures:
         //  - void functor(gcc_wrappers::type::base);
         //  - void functor(gcc_wrappers::type::base, size_t index);
         // You can have a return type if you like, but we'll ignore it.
         template<typename Functor>
         void for_each_argument_type(Functor&& functor) const;
   };
   using function_ptr = node_pointer_template<function>;
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"
#include "gcc_wrappers/type/function.inl"