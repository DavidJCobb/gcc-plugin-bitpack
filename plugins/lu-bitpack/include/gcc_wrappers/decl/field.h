#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/list_node.h"
#include "gcc_wrappers/type.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class field : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == FIELD_DECL;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(field)
            
         public:
            list_node attributes() const;
            type      member_of() const; // RECORD_TYPE or UNION_TYPE
            type      value_type() const;
            
            size_t offset_in_bits() const;
            size_t size_in_bits() const;
            
            // Indicates whether this field is a bitfield. If so, 
            // `size_in_bits` is the field width.
            bool is_bitfield() const;
      };
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"