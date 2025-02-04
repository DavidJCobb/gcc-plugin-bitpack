#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/expr/ternary.h"
#include "gcc_wrappers/type/base.h"

namespace gcc_wrappers {
   namespace flow {
      class simple_if_else_set {
         protected:
            expr::optional_ternary previous;
            type::base             type;
         
         public:
            simple_if_else_set(type::optional_base result_type = {});
            
            // This is what you'd want to append to a local block somewhere.
            expr::optional_ternary result;
            
            void add_branch(value condition, expr::base branch);
      };
   }
}