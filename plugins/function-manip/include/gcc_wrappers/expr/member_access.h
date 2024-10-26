#pragma once
#include "gcc_wrappers/expr/base.h"

namespace gcc_wrappers {
   namespace decl {
      class variable;
   }
}

namespace gcc_wrappers::expr {
   class member_access : public base {
      public:
         static bool tree_is(tree);
         
         void set_from_untyped(tree);
         
         static member_access create(
            decl::variable& object,
            const char*     field_name,
            location_t      access_location     = UNKNOWN_LOCATION,
            location_t      field_name_location = UNKNOWN_LOCATION,
            location_t      arrow_operator_loc  = UNKNOWN_LOCATION
         );
         
         tree object_untyped() const; // could be a DECL or EXPR
         tree field_untyped() const; // FIELD_DECL
   };
}