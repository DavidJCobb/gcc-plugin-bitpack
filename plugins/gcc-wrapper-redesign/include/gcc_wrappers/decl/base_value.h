#pragma once
#include <string_view>
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   namespace type {
      class base;
   }
   class scope;
}

namespace gcc_wrappers {
   namespace decl {
      //
      // DECL types that can be used as values. Note that you must 
      // explicitly cast them via `as_value`.
      //
      class base_value : public base {
         public:
            static bool raw_node_is(tree t);
            GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(base_value)
            
         public:
            bool linkage_status_unknown() const;
            
            type::base value_type() const;
            
            value as_value();
      };
      DECLARE_GCC_NODE_POINTER_WRAPPER(base_value);
   }
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"