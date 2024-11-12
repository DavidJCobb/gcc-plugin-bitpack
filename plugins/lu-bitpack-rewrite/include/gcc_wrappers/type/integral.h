#pragma once
#include "gcc_wrappers/type/numeric.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::expr {
   class integer_constant;
}

namespace gcc_wrappers::type {
   class integral : public numeric {
      public:
         static bool node_is(tree t) {
            if (t == NULL_TREE)
               return false;
            switch (TREE_CODE(t)) {
               case ENUMERAL_TYPE:
               case INTEGER_TYPE:
                  return true;
               default:
                  break;
            }
            return false;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(integral)
         
      public:
         size_t bitcount() const; // TYPE_PRECISION
         
         bool is_signed() const;
         bool is_unsigned() const;
         
         intmax_t minimum_value() const;
         uintmax_t maximum_value() const;
         
         integral make_signed() const;
         integral make_unsigned() const;
         
         bool can_hold_value(const expr::integer_constant) const;
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"