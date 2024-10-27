#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/expr/declare.h"
#include "gcc_wrappers/type.h"
#include "gcc_wrappers/value.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class variable : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == VAR_DECL;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(variable)
            
            value as_value() const {
               value v;
               v.set_from_untyped(this->_node);
               return v;
            }
            
         public:
            variable(
               const char* identifier_name,
               const type& value_type,
               location_t  source_location = UNKNOWN_LOCATION
            );
            
            expr::declare make_declare_expr();
            
            type value_type() const;
      };
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"