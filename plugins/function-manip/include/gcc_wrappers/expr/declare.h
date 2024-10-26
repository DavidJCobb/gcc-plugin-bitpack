#pragma once
#include "gcc_wrappers/expr/base.h"

namespace gcc_wrappers {
   namespace decl {
      class variable;
   }
   class type;
}

namespace gcc_wrappers::expr {
   class declare : public base {
      public:
         static bool tree_is(tree);
         
         void set_from_untyped(tree);
         
         static declare create(
            decl::variable&,
            location_t source_location = UNKNOWN_LOCATION
         );
   };
}