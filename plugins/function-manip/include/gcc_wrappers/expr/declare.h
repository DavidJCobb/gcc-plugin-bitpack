#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class variable;
   }
   namespace expr {
      class declare : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == DECL_EXPR;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(declare)
         
         public:
            declare(decl::variable&, location_t source_location = UNKNOWN_LOCATION);
      };
      static_assert(sizeof(declare) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"