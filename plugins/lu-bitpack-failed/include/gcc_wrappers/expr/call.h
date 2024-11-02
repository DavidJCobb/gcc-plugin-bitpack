#pragma once
#include <type_traits>
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/type.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   class value;
   
   namespace expr {
      class call : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == CALL_EXPR;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(call)
         
         public:
            template<typename... Args> requires (std::is_base_of_v<value, Args> && ...)
            call(decl::function func_decl, Args... args) {
               auto func_type = func_decl.function_type();
               if (!func_type.is_unprototyped_function()) {
                  //
                  // Verify argcount. Note that C does not support default argument 
                  // initializers, so a function that takes N args must receive N 
                  // explicitly-specified args.
                  //
                  if (func_type.is_varargs_function()) {
                     assert(sizeof...(Args) >= func_type.fixed_function_arg_count());
                  } else {
                     assert(sizeof...(Args) == func_type.fixed_function_arg_count());
                  }
               }
               this->_node = build_call_expr(
                  func_decl.as_untyped(),
                  sizeof...(Args),
                  args.as_untyped()...
               );
            }
      };
      static_assert(sizeof(assign) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"