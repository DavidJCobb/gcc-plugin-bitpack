#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/expr/declare.h"
#include "gcc_wrappers/expr/member_access.h"

namespace gcc_wrappers {
   class type;
}

namespace gcc_wrappers::decl {
   class variable : public base {
      public:
         static bool tree_is(tree);
         
         void set_from_untyped(tree);
         
         expr::declare make_declare_expr(location_t source_location = UNKNOWN_LOCATION);
         
         // Errors if the variable isn't a record or union.
         expr::member_access access_member(
            const char* field_name,
            location_t  access_location     = UNKNOWN_LOCATION,
            location_t  field_name_location = UNKNOWN_LOCATION,
            location_t  arrow_operator_loc  = UNKNOWN_LOCATION
         );
         
         static variable create(
            const char* identifier_name,
            const type& variable_type,
            location_t  source_location = UNKNOWN_LOCATION
         );
   };
}