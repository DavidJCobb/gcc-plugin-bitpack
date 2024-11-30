#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class type_def : public base {
         public:
            static bool raw_node_is(tree t) {
               return TREE_CODE(t) == TYPE_DECL;
            }
            GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(type_def)
            
         public:
            type::base_ptr declared() const;      // `b` in `typedef a b`
            type::base_ptr is_synonym_of() const; // `a` in `typedef a b`. return value may be empty.
      };
   }
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"