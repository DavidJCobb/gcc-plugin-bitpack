#pragma once
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers::decl {
   class label;
}

namespace gcc_wrappers::expr {
   class declare_label : public base {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == LABEL_EXPR;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(declare_label)
      
      public:
         declare_label(decl::label);
         
         decl::label declaration() const;
   };
   DECLARE_GCC_NODE_POINTER_WRAPPER(declare_label);
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"