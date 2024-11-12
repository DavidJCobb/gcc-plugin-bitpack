#pragma once
#include <optional>
#include <string>
#include <type_traits>
#include "lu/strings/zview.h"
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/type/floating_point.h"
#include "gcc_wrappers/_boilerplate.define.h"
#include "gcc_headers/plugin-version.h"

namespace gcc_wrappers {
   namespace expr {
      class integer_constant;
      
      class floating_point_constant : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == REAL_CST;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(floating_point_constant)
            
         public:
            // CAN_HAVE_LOCATION_P is false for all nodes of this type, so calls 
            // to this member function would replace `this->_node` with a pointer 
            // of a different type.
            void set_source_location(location_t, bool wrap_if_necessary = false);
            
         protected:
            HOST_WIDE_INT _to_host_wide_int() const;
         
         public:
            floating_point_constant() {}
            explicit floating_point_constant(type::floating_point, integer_constant);
            
            static floating_point_constant from_string(type::floating_point, lu::strings::zview);
            
            static floating_point_constant make_infinity(type::floating_point, bool sign = false);
            
            bool is_zero() const; // real_zerop
            bool is_one() const; // real_onep
            bool is_negative_one() const; // real_minus_onep
            
            bool is_any_infinity() const;
            bool is_signed_infinity(bool negative) const;
            bool is_nan() const;
            bool is_signalling_nam() const;
            
            bool is_negative() const;
            bool is_negative_zero() const;
            
            bool is_bitwise_identical(const floating_point_constant) const;
            
            std::string to_string() const;
            
            template<typename Integral> requires std::is_integral_v<Integral>
            Integral to_integer() const {
               return (Integral) _to_host_wide_int();
            }
            
            bool operator<(const floating_point_constant&) const;
            bool operator==(const floating_point_constant&) const;
      };
      static_assert(sizeof(floating_point_constant) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"