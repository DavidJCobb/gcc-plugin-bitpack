#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/expr/label.h"
#include "gcc_wrappers/value.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class label : public base {
         public:
            static bool raw_node_is(tree t) {
               return TREE_CODE(t) == LABEL_DECL;
            }
            GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(label)
            
         public:
            label(location_t source_location = UNKNOWN_LOCATION);
            
            expr::label make_label_expr();
            
            // You're allowed to take the address of a label when using 
            // GCC extensions.
            value address_of();
      };
      
      using label_ptr = node_pointer_template<label>;
   }
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"