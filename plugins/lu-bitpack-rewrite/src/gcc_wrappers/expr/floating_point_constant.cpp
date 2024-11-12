#include "gcc_wrappers/expr/floating_point_constant.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"
#include <real.h> // comparisons

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(floating_point_constant)
   
   HOST_WIDE_INT floating_point_constant::_to_host_wide_int() const {
      assert(!empty());
      return real_to_integer(TREE_REAL_CST_PTR(this->_node));
   }
   
   /*explicit*/ floating_point_constant::floating_point_constant(type::floating_point t, integer_constant n) {
      this->_node = build_real_from_int_cst(t.as_untyped(), n.as_untyped());
   }

   /*static*/ floating_point_constant floating_point_constant::from_string(type::floating_point type, lu::strings::zview str) {
      REAL_VALUE_TYPE raw;
      int overflow = real_from_string(&raw, str.c_str());
      
      // TODO: Set TREE_OVERFLOW(node) based on `overflow != 0`?
      return floating_point_constant::from_untyped(build_real(type.as_untyped(), raw));
   }
   
   /*static*/ floating_point_constant floating_point_constant::make_infinity(type::floating_point type, bool sign) {
      REAL_VALUE_TYPE raw;
      #if GCCPLUGIN_VERSION_MAJOR >= 13
         real_inf(&raw, sign);
      #else
         real_inf(&raw);
         raw.sign = sign;
      #endif
      return floating_point_constant::from_untyped(build_real(type.as_untyped(), raw));
   }

   bool floating_point_constant::is_zero() const {
      return real_zerop(this->_node);
   }
   bool floating_point_constant::is_one() const {
      return real_onep(this->_node);
   }
   bool floating_point_constant::is_negative_one() const {
      return real_minus_onep(this->_node);
   }
   
   // checks from real.h
   bool floating_point_constant::is_any_infinity() const {
      return !empty() && real_isinf(TREE_REAL_CST_PTR(this->_node));
   }
   bool floating_point_constant::is_signed_infinity(bool negative) const {
      if (empty())
         return false;
      auto* raw = TREE_REAL_CST_PTR(this->_node);
      #if GCCPLUGIN_VERSION_MAJOR >= 13
         return real_isinf(raw, negative);
      #else
         if (!real_isinf(raw))
            return false;
         return raw->sign == negative;
      #endif
   }
   bool floating_point_constant::is_nan() const {
      return !empty() && real_isnan(TREE_REAL_CST_PTR(this->_node));
   }
   bool floating_point_constant::is_signalling_nam() const {
      return !empty() && real_issignaling_nan(TREE_REAL_CST_PTR(this->_node));
   }
   
   bool floating_point_constant::is_negative() const {
      return !empty() && real_isneg(TREE_REAL_CST_PTR(this->_node));
   }
   bool floating_point_constant::is_negative_zero() const {
      return !empty() && real_isnegzero(TREE_REAL_CST_PTR(this->_node));
   }
   
   bool floating_point_constant::is_bitwise_identical(const floating_point_constant other) const {
      if (empty())
         return other.empty();
      else if (other.empty())
         return false;
      return real_identical(TREE_REAL_CST_PTR(this->_node), TREE_REAL_CST_PTR(other._node));
   }
   
   std::string floating_point_constant::to_string() const {
      std::string result;
      result.resize(64);
      real_to_decimal(
         result.data(),
         TREE_REAL_CST_PTR(this->_node),
         result.size(),
         0,   // max digits; 0 for no limit
         true // crop trailing zeroes
      );
      {
         size_t i = result.find_first_of('\0');
         if (i != std::string::npos)
            result.resize(i);
      }
      return result;
   }
   
   bool floating_point_constant::operator<(const floating_point_constant& other) const {
      assert(!empty());
      assert(!other.empty());
      return real_less(TREE_REAL_CST_PTR(this->_node), TREE_REAL_CST_PTR(other._node));
   }
   bool floating_point_constant::operator==(const floating_point_constant& other) const {
      assert(!empty());
      assert(!other.empty());
      return real_equal(TREE_REAL_CST_PTR(this->_node), TREE_REAL_CST_PTR(other._node));
   }
}