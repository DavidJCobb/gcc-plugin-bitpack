#pragma once
#include "gcc_wrappers/_wrapped_tree_node.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/enumeration.h"
#include "gcc_wrappers/type/floating_point.h"
#include "gcc_wrappers/type/integral.h"

namespace gcc_wrappers {
   namespace decl {
      class base;
   }
   namespace expr {
      class base;
   }
   
   class value;
   
   template<typename Decl>
   concept declaration_is_value = requires (const Decl& v) {
      requires std::is_base_of_v<decl::base, Decl>;
      { v.as_value() } -> std::same_as<value>;
   };
   
   // Base class for `*_EXPR`, `VAR_DECL`, etc.
   class value : public _wrapped_tree_node {
      public:
         template<typename T> requires declaration_is_value<T>
         T as_decl() const {
            assert(T::node_is(this->_node));
            T out;
            out._node = this->_node;
            return out;
         }
         
         template<typename T> requires std::is_base_of_v<expr::base, T>
         T as_expr() const {
            assert(T::node_is(this->_node));
            T out;
            out._node = this->_node;
            return out;
         }
         
      protected:
         value _make_arith1(tree_code expr_type);
         value _make_arith2(tree_code expr_type, value other);
         value _make_bitwise2(tree_code expr_type, value other);
         value _make_cmp2(tree_code expr_type, value other);
      
      public:
         // Value/result ype.
         type::base value_type() const;
         
         // Creates a COMPONENT_REF.
         // This value must be of a RECORD_TYPE or UNION_TYPE.
         value access_member(const char*);
         
         // Creates a ARRAY_REF.
         // This value must be of an ARRAY_TYPE. Index must be an INTEGER_TYPE.
         value access_array_element(value index);
         
         // Creates a ARRAY_RANGE_REF.
         // This value must be of an ARRAY_TYPE.
         value access_array_range(value start, value length);
         
         // Returns a value of a POINTER_TYPE.
         value address_of();
         
         // `a->b` is `a.deference().access_member("b")`
         // This value must be a POINTER_TYPE or REFERENCE_TYPE.
         value dereference();
         
         bool is_lvalue() const;
         
         //#pragma region Constant integer checks
            // Check if EXPR is a zero-value INTEGER_CST, VECTOR_CST, or 
            // COMPLEX_CST.
            bool is_constant_int_zero() const;
            bool is_constant_int_nonzero() const;
            bool is_constant_int_one() const;
            bool is_constant_int_neg_one() const;
            
            // for VECTOR_CST, this isn't quite the same as being "one."
            bool is_constant_true() const;
            
            // Check if EXPR is an INTEGER_CST, VECTOR_CST, or COMPLEX_CST 
            // for which all defined bits are set.
            bool is_constant_int_all_bits_set() const;
            
            // compare to C++ <bit> functions
            bool constant_has_single_bit() const;
            int constant_bit_floor() const;
         //#pragma endregion
         
         //#pragma region Constant float/real checks
            bool is_constant_real_zero() const;
         //#pragma endregion
         
         //
         // Functions below create expressions, treating `this` as a unary 
         // operand or the lefthand operand (as appropriate). The functions 
         // assert that type requirements are met.
         //
         
         //#pragma region Conversions
            bool convertible_to_enum(type::enumeration) const;
            bool convertible_to_floating_point() const;
            bool convertible_to_integer(type::integral) const;
            bool convertible_to_pointer() const;
            bool convertible_to_vector(type::base) const;
            
            value convert_to_enum(type::enumeration);
            value convert_to_floating_point(type::floating_point);
            value convert_to_integer(type::integral);
            value convert_to_pointer(type::pointer);
            
            // Optimize an expr for use as the condition of an if-statement, 
            // while-statement, ternary, etc..
            value convert_to_truth_value();
            
            // u8[n] -> u8*
            value convert_array_to_pointer();
            
            // Use for:
            //  - Explicitly casting away constness of pointers
            //  - Explicit casts between different pointer types
            value conversion_sans_bytecode(type::base);
         //#pragma endregion
         
         //#pragma region Comparison operators
            value cmp_is_less(value);             // LT_EXPR
            value cmp_is_less_or_equal(value);    // LE_EXPR
            value cmp_is_greater(value);          // GT_EXPR
            value cmp_is_greater_or_equal(value); // GE_EXPR
            value cmp_is_equal(value);            // EQ_EXPR
            value cmp_is_not_equal(value);        // NE_EXPR
         //#pragma endregion
         
         // Arithmetic operators. If you try mixing ints and floats, we'll 
         // convert the non-`this` type to match the `this` type.
         //
         // TODO: Overloads taking an integer constant.
         //#pragma region Arithmetic operators
            value arith_abs(); // ABS_EXPR
            value arith_neg(); // NEGATE_EXPR
            
            value decrement_pre(value by);  // PREDECREMENT_EXPR
            value decrement_post(value by); // POSTDECREMENT_EXPR
            value increment_pre(value by);  // PREINCREMENT_EXPR
            value increment_post(value by); // POSTINCREMENT_EXPR
            
            value add(value);
            value sub(value);
            value mul(value);
            value mod(value); // TRUNC_MOD_EXPR
            
            value div(value); // TRUNC_DIV_EXPR or RDIV_EXPR, depending on this type and other type
         //#pragma endregion
         
         //#pragma region Bitwise operators, available only on integers
            value bitwise_and(value other); // BIT_AND_EXPR
            value bitwise_not(); // BIT_NOT_EXPR
            value bitwise_or(value other);  // BIT_IOR_EXPR
            value bitwise_xor(value other); // BIT_XOR_EXPR
            value shift_left(value bitcount); // LSHIFT_EXPR
            value shift_right(value bitcount); // RSHIFT_EXPR
         //#pragma endregion
         
         //#pragma region Logical operators
            value logical_not(); // TRUTH_NOT_EXPR
            value logical_and(value, bool short_circuit = true); // TRUTH_ANDIF_EXPR or TRUTH_AND_EXPR
            value logical_or(value, bool short_circuit = true); // TRUTH_ORIF_EXPR or TRUTH_OR_EXPR
            value logical_xor(value); // TRUTH_XOR_EXPR. never short-circuits
         //#pragma endregion
   };
}