#pragma once
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::type {
   class numeric : public base {
      public:
         static bool node_is(tree t) {
            if (t == NULL_TREE)
               return false;
            switch (TREE_CODE(t)) {
               case COMPLEX_TYPE:
               case ENUMERAL_TYPE:
               case FIXED_POINT_TYPE:
               case INTEGER_TYPE:
               case REAL_TYPE:
                  return true;
               default:
                  break;
            }
            return false;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(numeric)
         
      public:
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"