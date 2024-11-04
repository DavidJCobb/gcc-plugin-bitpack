#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class type_def : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == TYPE_DECL;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(type_def)
            
         public:
            type::base declared() const;      // `b` in `typedef a b`
            type::base is_synonym_of() const; // `a` in `typedef a b`. return value may be empty().
      };
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"