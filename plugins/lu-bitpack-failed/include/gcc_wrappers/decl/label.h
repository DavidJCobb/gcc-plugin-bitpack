#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/expr/label.h"
#include "gcc_wrappers/value.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class label : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == LABEL_DECL;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(label)
            
            // You're allowed to take the address of a label when using 
            // GCC extensions.
            value as_value() const {
               value v;
               v.set_from_untyped(this->_node);
               return v;
            }
            
         public:
            label(
               location_t source_location = UNKNOWN_LOCATION
            );
            
            expr::label make_label_expr();
      };
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"