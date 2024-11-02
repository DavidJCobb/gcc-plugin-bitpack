#include "gcc_wrappers/value.h"
#include <cassert>
#include "gcc_headers/plugin-version.h"
#include <c-tree.h> // build_component_ref
#include <convert.h> // convert_to_integer and friends
#include <stringpool.h> // get_identifier

#include "gcc_helpers/gcc_version_info.h"

namespace gcc_wrappers {
   type value::value_type() const {
      return type::from_untyped(TREE_TYPE(this->_node));
   }
   
   value value::access_member(const char* name) {
      auto vt = this->value_type();
      assert(vt.is_record() || vt.is_union());
      
      auto field_identifier = get_identifier(name);
      
      value out;
      out._node = build_component_ref(
         UNKNOWN_LOCATION, // source location: overall member-access operation
         this->_node,      // object to access on
         field_identifier, // member to access, as an IDENTIFIER_NODE
         UNKNOWN_LOCATION  // source location: field identifier
         #if GCCPLUGIN_VERSION_MAJOR >= 9 && GCCPLUGIN_VERSION_MINOR >= 5
            // GCC PR91134
            // https://github.com/gcc-mirror/gcc/commit/7a3ee77a2e33b8b8ad31aea27996ebe92a5c8d83
            ,
            UNKNOWN_LOCATION // source location: arrow operator, if present
            #if GCCPLUGIN_VERSION_MAJOR >= 13 && GCCPLUGIN_VERSION_MINOR >= 3
               // https://github.com/gcc-mirror/gcc/commit/bb49b6e4f55891d0d8b596845118f40df6ae72a5
               ,
               true // default; todo: use false for offsetof, etc.?
            #endif
         #endif
      );
      assert(TREE_CODE(out._node) == COMPONENT_REF);
      return out;
   }
   
   value value::access_array_element(value index) {
      assert(this->value_type().is_array());
      assert(index.value_type().is_integer());
      value out;
      out.set_from_untyped(build_array_ref(
         UNKNOWN_LOCATION,
         this->_node,
         index.as_untyped()
      ));
      return out;
   }
   
   // Creates a ARRAY_RANGE_REF.
   // This value must be of an ARRAY_TYPE.
   //value value::access_array_range(value start, value length); // TODO
   
   // Returns a value of a POINTER_TYPE.
   value value::address_of() {
      value out;
      out.set_from_untyped(build1(
         ADDR_EXPR,
         this->value_type().add_pointer().as_untyped(),
         this->_node
      ));
      return out;
   }
   
   value value::dereference() {
      auto vt = this->value_type();
      assert(vt.is_pointer()); // TODO: allow reference types too
      
      value out;
      out.set_from_untyped(build1(
         INDIRECT_REF,
         this->value_type().remove_pointer().as_untyped(),
         this->_node
      ));
      return out;
   }
   
   bool value::is_lvalue() const {
      return lvalue_p(this->_node); // c-family/c-common.h, c/c-typeck.cc
   }
   
   //#pragma region Constant integer checks
      bool value::is_constant_int_zero() const {
         return integer_zerop(this->_node); // tree.h, tree.cc
      }
      bool value::is_constant_int_nonzero() const {
         return integer_nonzerop(this->_node); // tree.h, tree.cc
      }
      bool value::is_constant_int_one() const {
         return integer_onep(this->_node); // tree.h, tree.cc
      }
      bool value::is_constant_int_neg_one() const {
         return integer_minus_onep(this->_node); // tree.h, tree.cc
      }
      
      bool value::is_constant_true() const {
         return integer_truep(this->_node); // tree.h, tree.cc
      }
      
      bool value::is_constant_int_all_bits_set() const {
         return integer_all_onesp(this->_node); // tree.h, tree.cc
      }
      
      bool value::constant_has_single_bit() const {
         return integer_pow2p(this->_node); // tree.h, tree.cc
      }
      int value::constant_bit_floor() const {
         return tree_floor_log2(this->_node); // tree.h, tree.cc
      }
   //#pragma endregion
         
   //#pragma region Constant float/real checks
      bool value::is_constant_real_zero() const {
         return real_zerop(this->_node); // tree.h, tree.cc
      }
   //#pragma endregion
   
   //
   // Functions below create expressions, treating `this` as a unary 
   // operand or the lefthand operand (as appropriate). The functions 
   // assert that type requirements are met.
   //
   
   //#pragma region Conversions
      bool value::convertible_to_enum(type desired) const {
         return this->convertible_to_integer(desired);
      }
      bool value::convertible_to_floating_point() const {
         // per `convert_to_real_1 `gcc/convert.cc`
         if (TREE_CODE(this->_node) == COMPOUND_EXPR) {
            value wrap;
            wrap.set_from_untyped(TREE_OPERAND(this->_node, 1));
            return wrap.convertible_to_floating_point();
         }
         switch (this->value_type().code()) {
            case REAL_TYPE:
               return true;
            case INTEGER_TYPE:
            case ENUMERAL_TYPE:
            case BOOLEAN_TYPE:
            #if IS_BITINT_TYPE_DEFINED
            case BITINT_TYPE:
            #endif
               return true;
            case FIXED_POINT_TYPE:
               return true;
            case COMPLEX_TYPE:
               return true;
         }
         return false;
      }
      bool value::convertible_to_integer(type desired) const {
         // per `convert_to_integer_1 `gcc/convert.cc`
         if (desired.is_enum()) {
            if (!desired.is_complete())
               return false;
         }
         if (TREE_CODE(this->_node) == COMPOUND_EXPR) {
            value wrap;
            wrap._node = TREE_OPERAND(this->_node, 1);
            return wrap.convertible_to_integer(desired);
         }
         const auto vt = this->value_type();
         switch (vt.code()) {
            case POINTER_TYPE:
            case REFERENCE_TYPE:
               return true;
            case INTEGER_TYPE:
            case ENUMERAL_TYPE:
            case BOOLEAN_TYPE:
            case OFFSET_TYPE:
            #if IS_BITINT_TYPE_DEFINED
            case BITINT_TYPE:
            #endif
               return true;
            case REAL_TYPE:
               return true;
            case FIXED_POINT_TYPE:
               return true;
            case COMPLEX_TYPE:
               // comparing type sizes always tests as non-equal if neither is an integer constant
               if (!vt.is_complete() || !desired.is_complete()) {
                  return false;
               }
               if (vt.size_in_bits() != desired.size_in_bits()) {
                  return false;
               }
               return true;
         }
         return false;
      }
      bool value::convertible_to_pointer() const {
         // per `convert_to_pointer_1` in `gcc/convert.cc`
         switch (this->value_type().code()) {
            case POINTER_TYPE:
            case REFERENCE_TYPE:
               return true;
            case INTEGER_TYPE:
            case ENUMERAL_TYPE:
            case BOOLEAN_TYPE:
            #if IS_BITINT_TYPE_DEFINED
            case BITINT_TYPE:
            #endif
               return true;
         }
         return false;
      }
      bool value::convertible_to_vector(type desired) const {
         // per `convert_to_vector` in `gcc/convert.cc`
         const auto vt = this->value_type();
         switch (vt.code()) {
            case INTEGER_TYPE:
            case VECTOR_TYPE:
               // comparing type sizes always tests as non-equal if neither is an integer constant
               if (!vt.is_complete() || !desired.is_complete()) {
                  return false;
               }
               if (vt.size_in_bits() != desired.size_in_bits()) {
                  return false;
               }
               return true;
         }
         return false;
         
      }
      
      value value::convert_to_enum(type desired) {
         assert(desired.is_enum());
         assert(convertible_to_enum(desired));
         value out;
         out.set_from_untyped(::convert_to_integer(desired.as_untyped(), this->_node));
         assert(out.as_untyped() != error_mark_node); // if this fails, update the `convertible_to...` func
         return out;
      }
      value value::convert_to_floating_point(type desired) {
         assert(convertible_to_floating_point());
         value out;
         out.set_from_untyped(::convert_to_real(desired.as_untyped(), this->_node));
         assert(out.as_untyped() != error_mark_node); // if this fails, update the `convertible_to...` func
         return out;
      }
      value value::convert_to_integer(type desired) {
         assert(desired.is_integer());
         assert(convertible_to_integer(desired));
         value out;
         out.set_from_untyped(::convert_to_integer(desired.as_untyped(), this->_node));
         assert(out.as_untyped() != error_mark_node); // if this fails, update the `convertible_to...` func
         return out;
      }
      value value::convert_to_pointer(type desired) {
         assert(convertible_to_pointer());
         value out;
         out.set_from_untyped(::convert_to_pointer(desired.as_untyped(), this->_node));
         assert(out.as_untyped() != error_mark_node);
         return out;
      }

      value value::convert_to_truth_value() {
         assert(!empty());
         value v;
         v.set_from_untyped(c_common_truthvalue_conversion(UNKNOWN_LOCATION, this->_node));
         return v;
      }
      
      value value::convert_array_to_pointer() {
         //
         // In mimicry of `array_to_pointer_conversion` in `c/c-typeck.cc`.
         //
         auto array_type = this->value_type();
         assert(array_type.is_array());
         STRIP_TYPE_NOPS(this->_node);
         auto value_type = array_type.array_value_type();
         
         [[maybe_unused]] bool variably_modified = false;
         #ifdef C_TYPE_VARIABLY_MODIFIED
            variably_modified = C_TYPE_VARIABLY_MODIFIED(value_type.to_untyped());
         #endif
         
         auto pointer_type = value_type.add_pointer();
         #ifdef C_TYPE_VARIABLY_MODIFIED
            C_TYPE_VARIABLY_MODIFIED(pointer_type) = variably_modified;
         #endif
         
         if (INDIRECT_REF_P(this->_node)) {
            value v;
            v.set_from_untyped(
               convert(
                  pointer_type.as_untyped(),
                  TREE_OPERAND(this->_node, 0)
               )
            );
            return v;
         }
         
         value v;
         v.set_from_untyped(
            convert(
               pointer_type.as_untyped(),
               build_unary_op(
                  UNKNOWN_LOCATION,
                  ADDR_EXPR,
                  this->_node,
                  true // suppress conversion within `build_unary_op` since we do it with `convert`
               )
            )
         );
         return v;
      }
      
      value value::conversion_sans_bytecode(type desired) {
         value out;
         out.set_from_untyped(build1(
            NOP_EXPR,
            desired.as_untyped(),
            this->_node
         ));
         return out;
      }
   //#pragma endregion
   
   value value::_make_cmp2(tree_code expr_type, value other) {
      //
      // TODO: permit pointer operands?
      //
      auto vt_t = this->value_type();
      auto vt_b = other.value_type();
      assert(vt_t.is_arithmetic());
      assert(vt_b.is_arithmetic());
      
      value out;
      out.set_from_untyped(build2(
         expr_type,
         boolean_type_node,
         this->_node,
         other._node
      ));
      return out;
   }
   
   //#pragma region Comparison operators
      value value::cmp_is_less(value other) {
         return _make_cmp2(LT_EXPR, other);
      }
      value value::cmp_is_less_or_equal(value other) {
         return _make_cmp2(LE_EXPR, other);
      }
      value value::cmp_is_greater(value other) {
         return _make_cmp2(GT_EXPR, other);
      }
      value value::cmp_is_greater_or_equal(value other) {
         return _make_cmp2(GE_EXPR, other);
      }
      value value::cmp_is_equal(value other) {
         return _make_cmp2(EQ_EXPR, other);
      }
      value value::cmp_is_not_equal(value other) {
         return _make_cmp2(NE_EXPR, other);
      }
   //#pragma endregion
   
   value value::_make_arith1(tree_code expr_type) {
      assert(this->value_type().is_arithmetic());
      value out;
      out.set_from_untyped(build1(
         expr_type,
         this->value_type().as_untyped(),
         this->_node
      ));
      return out;
   }
   
   // Helper function for arith operations, requiring that both operands 
   // be arithmetic types and converting the second (`other`) operand to 
   // a type of the same TREE_CODE as the first's type, as needed.
   value value::_make_arith2(tree_code expr_type, value other) {
      auto vt_t = this->value_type();
      auto vt_b = other.value_type();
      assert(vt_t.is_arithmetic());
      assert(vt_b.is_arithmetic());
      
      value arg = other;
      if (vt_t.is_integer()) {
         if (!vt_b.is_integer())
            arg = arg.convert_to_integer(vt_t);
      } else if (vt_t.is_floating_point()) {
         if (!vt_b.is_floating_point())
            arg = arg.convert_to_floating_point(vt_t);
      }
      
      value out;
      out.set_from_untyped(build2(
         expr_type,
         vt_t.as_untyped(),
         this->_node,
         arg._node
      ));
      return out;
   }
   
   value value::_make_bitwise2(tree_code expr_type, value other) {
      auto vt = this->value_type();
      assert(vt.is_integer());
      assert(other.value_type().is_integer());
      
      value out;
      out.set_from_untyped(build2(
         expr_type,
         vt.as_untyped(),
         this->_node,
         other._node
      ));
      return out;
   }
   
   //#pragma region Arithmetic operators
      value value::arith_abs() {
         return _make_arith1(ABS_EXPR);
      }
      value value::arith_neg() {
         return _make_arith1(NEGATE_EXPR);
      }
      
      value value::decrement_pre(value by) {
         return _make_arith2(PREDECREMENT_EXPR, by);
      }
      value value::decrement_post(value by) {
         return _make_arith2(POSTDECREMENT_EXPR, by);
      }
      value value::increment_pre(value by) {
         return _make_arith2(PREINCREMENT_EXPR, by);
      }
      value value::increment_post(value by) {
         return _make_arith2(POSTINCREMENT_EXPR, by);
      }
      
      value value::add(value other) {
         return _make_arith2(PLUS_EXPR, other);
      }
      value value::sub(value other) {
         return _make_arith2(MINUS_EXPR, other);
      }
      value value::mul(value other) {
         return _make_arith2(MULT_EXPR, other);
      }
      //value value::mod(value other); // TRUNC_MOD_EXPR
      
      //value value::div(value other); // TRUNC_DIV_EXPR or RDIV_EXPR, depending on this type and other type
   //#pragma endregion
   
   //#pragma region Bitwise operators, available only on integers
      value value::bitwise_and(value other) {
         return _make_bitwise2(BIT_AND_EXPR, other);
      }
      value value::bitwise_not() {
         auto vt = this->value_type();
         assert(vt.is_integer());
         value out;
         out.set_from_untyped(build1(
            BIT_NOT_EXPR,
            vt.as_untyped(),
            this->_node
         ));
         return out;
      }
      value value::bitwise_or(value other) {
         return _make_bitwise2(BIT_IOR_EXPR, other);
      }
      value value::bitwise_xor(value other) {
         return _make_bitwise2(BIT_XOR_EXPR, other);
      }
      value value::shift_left(value bitcount) {
         return _make_bitwise2(LSHIFT_EXPR, bitcount);
      }
      value value::shift_right(value bitcount) {
         return _make_bitwise2(RSHIFT_EXPR, bitcount);
      }
   //#pragma endregion
   
   //#pragma region Logical operators
      value value::logical_not() {
         value out;
         out.set_from_untyped(build1(
            TRUTH_NOT_EXPR,
            truthvalue_type_node,
            c_common_truthvalue_conversion(UNKNOWN_LOCATION, this->_node)
         ));
         return out;
      }
      value value::logical_and(value other, bool short_circuit) {
         const auto code = short_circuit ? TRUTH_ANDIF_EXPR : TRUTH_AND_EXPR;
      
         value out;
         out.set_from_untyped(build2(
            code,
            truthvalue_type_node,
            c_common_truthvalue_conversion(UNKNOWN_LOCATION, this->_node),
            c_common_truthvalue_conversion(UNKNOWN_LOCATION, other.as_untyped())
         ));
         return out;
      }
      value value::logical_or(value other, bool short_circuit) {
         const auto code = short_circuit ? TRUTH_ORIF_EXPR : TRUTH_OR_EXPR;
      
         value out;
         out.set_from_untyped(build2(
            code,
            truthvalue_type_node,
            c_common_truthvalue_conversion(UNKNOWN_LOCATION, this->_node),
            c_common_truthvalue_conversion(UNKNOWN_LOCATION, other.as_untyped())
         ));
         return out;
      }
      value value::logical_xor(value other) {
         value out;
         out.set_from_untyped(build2(
            TRUTH_XOR_EXPR,
            truthvalue_type_node,
            c_common_truthvalue_conversion(UNKNOWN_LOCATION, this->_node),
            c_common_truthvalue_conversion(UNKNOWN_LOCATION, other.as_untyped())
         ));
         return out;
      }
   //#pragma endregion
}