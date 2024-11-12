#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/expr/declare.h"
#include "gcc_wrappers/type/base.h"
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
            variable() {}
            variable(
               const char*       identifier_name,
               const type::base& value_type,
               location_t        source_location = UNKNOWN_LOCATION
            );
            
            expr::declare make_declare_expr();
            
            type::base value_type() const;
            
            value initial_value() const;
            void set_initial_value(value);
            
            bool is_defined_elsewhere() const;
            void set_is_defined_elsewhere(bool);
            
            // Indicates that this decl's name is to be accessible from 
            // outside of the current translation unit.
            bool is_externally_accessible() const; // TREE_PUBLIC
            void make_externally_accessible();
            void set_is_externally_accessible(bool);
            
            void make_file_scope_static();
      };
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"