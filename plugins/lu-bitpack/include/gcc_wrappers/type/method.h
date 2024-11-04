#pragma once
#include "gcc_wrappers/type/function.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::type {
   class method : public function {
      public:
         static bool node_is(tree t) {
            return t != NULL_TREE && TREE_CODE(t) == METHOD_TYPE;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(method)
         
      public:
         base is_method_of() const;
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"