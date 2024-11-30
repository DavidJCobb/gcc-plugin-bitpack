#pragma once
#include "gcc_wrappers/type/function.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers::type {
   class method : public function {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == METHOD_TYPE;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(method)
         
      public:
         base is_method_of() const;
   };
   using method_ptr = node_pointer_template<method>;
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"