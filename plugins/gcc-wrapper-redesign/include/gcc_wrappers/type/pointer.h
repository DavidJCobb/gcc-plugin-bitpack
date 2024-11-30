#pragma once
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers::type {
   class pointer : public base {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == POINTER_TYPE;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(pointer)
         
      public:
         bool compatible_address_space_with(pointer) const;
         bool same_target_and_address_space_as(pointer) const;
   };
   using pointer_ptr = node_pointer_template<pointer>;
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"