#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/statement_list.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      //
      // NOTE: Our current design for generating GENERIC from scratch is:
      //
      //  - We defer creation of a BIND_EXPR's BLOCK node until the BIND_EXPR 
      //    (or an ancestor BIND_EXPR) is made the root block of a FUNCTION_DECL.
      //
      //  - We likewise defer setting BIND_EXPR_VARS until a block is assigned.
      //
      class local_block : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == BIND_EXPR;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(local_block)
         
         public:
            local_block(); // void-type with allocated statement list
            local_block(type); // with allocated statement list
            local_block(statement_list&&);
            local_block(type, statement_list&&);
            
            statement_list statements();
            
            // for editing function decls.
            void set_block_node(tree block);
      };
      static_assert(sizeof(local_block) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"