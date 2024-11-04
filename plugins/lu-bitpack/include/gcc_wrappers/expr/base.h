#pragma once
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/value.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      class base : public value {
         public:
            static bool node_is(tree t) {
               return EXPR_P(t);
            }
            WRAPPED_TREE_NODE_BOILERPLATE(base)
         
         public:
            location_t source_location() const;
         
            type::base get_result_type() const;
            
            bool suppresses_unused_warnings();
            void suppress_unused_warnings();
            void set_suppresses_unused_warnings(bool);
      };
      static_assert(sizeof(base) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"