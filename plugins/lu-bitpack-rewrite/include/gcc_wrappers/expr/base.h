#pragma once
#include <type_traits>
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/value.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      class base : public value {
         public:
            static bool node_is(tree t) {
               return EXPR_P(t) || CONSTANT_CLASS_P(t);
            }
            WRAPPED_TREE_NODE_BOILERPLATE(base)
         
         public:
            location_t source_location() const;
            
            // Subclasses that represent *_CST nodes must `delete` this function.
            void set_source_location(location_t, bool wrap_if_necessary = false);
         
            type::base get_result_type() const;
            
            // Given a constant expression, return a copy that has been as simplified 
            // as possible. If `manifestly_constant_evaluated` is true, then we treat 
            // the expression as manifestly constant evaluated (i.e. evaluated in a 
            // context where C++ `std::is_constant_evaluated()` would be `true`).
            base folded(bool manifestly_constant_evaluated = false) const;
            
            // Returns empty if not a constant.
            base fold_to_c_constant(
               bool  initializer,
               bool  as_lvalue
            ) const;
            
            bool suppresses_unused_warnings();
            void suppress_unused_warnings();
            void set_suppresses_unused_warnings(bool);
         
            bool is_associative_operator() const; // a + b + c == (a + b) + c == a + (b + c)
            bool is_commutative_operator() const; // a + b == b + a
            size_t operator_operand_count() const; // number of operands allowed, not number actually present
      };
      static_assert(sizeof(base) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"