#pragma once
#include <type_traits>
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/value.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   namespace constant {
      class base : public value {
         public:
            static bool raw_node_is(tree t) {
               return CONSTANT_CLASS_P(t);
            }
            GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(base)
         
         public:
            type::base value_type() const;
      };
      DECLARE_GCC_NODE_POINTER_WRAPPER(base);
   }
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"