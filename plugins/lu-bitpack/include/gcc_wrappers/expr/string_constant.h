#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      class string_constant : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == STRING_CST;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(string_constant)
         
         public:
            string_constant(std::string_view);
            
            std::string value() const;
      };
      static_assert(sizeof(string_constant) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"