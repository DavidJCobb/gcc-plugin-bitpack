#pragma once
#include "lu/strings/zview.h"
#include "gcc_wrappers/decl/base_value.h"
#include "gcc_wrappers/expr/declare.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/value.h"
#include "gcc_wrappers/_node_boilerplate.define.h"

namespace gcc_wrappers {
   class identifier;
}

namespace gcc_wrappers::decl {
   class variable : public base_value {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == VAR_DECL;
         }
         GCC_NODE_WRAPPER_BOILERPLATE(variable)
         
      public:
         variable(
            lu::strings::zview identifier_name,
            type::base&        value_type,
            location_t         source_location = UNKNOWN_LOCATION
         );
         variable(
            identifier  identifier_node,
            type::base& value_type,
            location_t  source_location = UNKNOWN_LOCATION
         );
         
         expr::declare make_declare_expr();
         
         bool is_defined_elsewhere() const;
         void set_is_defined_elsewhere(bool);
         
         // Indicates that this decl's name is to be accessible from 
         // outside of the current translation unit.
         bool is_externally_accessible() const; // TREE_PUBLIC
         void make_externally_accessible();
         void set_is_externally_accessible(bool);
         
         void make_file_scope_static();
   };
   DECLARE_GCC_OPTIONAL_NODE_WRAPPER(variable);
}

#include "gcc_wrappers/_node_boilerplate.undef.h"