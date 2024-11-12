#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/value.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class param : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == PARM_DECL;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(param)
            
            value as_value() const {
               value v;
               v.set_from_untyped(this->_node);
               return v;
            }
            
         public:
            type::base value_type() const;
            type::base effective_type() const;
      };
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"