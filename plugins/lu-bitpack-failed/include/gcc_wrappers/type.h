#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include "gcc_wrappers/_wrapped_tree_node.h"
#include "gcc_wrappers/list_node.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   class type : public _wrapped_tree_node {
      public:
         static bool node_is(tree t) {
            return TYPE_P(t);
         }
         WRAPPED_TREE_NODE_BOILERPLATE(type)
         
      public:
         struct enum_member {
            std::string_view name;
            intmax_t         value = 0;
         };
         struct fixed_point_info {
            struct {
               size_t fractional = 0; // TYPE_FBIT
               size_t integral   = 0; // TYPE_IBIT
               size_t total      = 0; // TYPE_PRECISION
            } bitcounts;
            bool      is_saturating = false;
            bool      is_signed     = false;
            intmax_t  min = 0; // TYPE_MIN_VALUE
            uintmax_t max = 0; // TYPE_MAX_VALUE
         };
         struct floating_point_info {
            size_t bitcount = 0;
         };
         struct integral_info {
            size_t    bitcount  = 0; // TYPE_PRECISION
            bool      is_signed = false; // for enums, only true if any member is negative
            intmax_t  min = 0; // for enums, lowest member; for integers, lowest representable value
            uintmax_t max = 0; // for enums, highest member; for integers, highest representable value
         };
         
      public:
         type() {}
         
         template<typename... Args> requires (std::is_same_v<Args, type> && ...)
         static type make_function_type(type return_type, Args... args) {
            type t;
            t._node = build_function_type_list( // tree.h
               return_type.as_untyped(),
               args.as_untyped()...,
               NULL_TREE
            );
            return t;
         };
         
         std::string pretty_print() const;
         
         list_node attributes() const;
         
         type canonical() const;
         type main_variant() const;
         
         bool is_complete() const;
         
         size_t size_in_bits() const;
         size_t size_in_bytes() const;
         
         bool is_arithmetic() const; // INTEGER_TYPE or REAL_TYPE
         bool is_array() const;
         bool is_boolean() const;
         bool is_enum() const; // ENUMERAL_TYPE
         bool is_fixed_point() const;
         bool is_floating_point() const; // REAL_TYPE
         bool is_function() const; // FUNCTION_TYPE or METHOD_TYPE
         bool is_integer() const; // does not include enums or bools
         bool is_method() const; // METHOD_TYPE
         bool is_record() const; // class or struct
         bool is_union() const;
         bool is_void() const;
         
         bool is_signed() const;
         bool is_unsigned() const;
         bool is_saturating() const;
         
         type add_const() const;
         type remove_const() const;
         bool is_const() const;
         
         type add_pointer() const;
         type remove_pointer() const;
         bool is_pointer() const;
         
         type make_signed() const;
         type make_unsigned() const;
         
         type add_array_extent(size_t) const;
         
         // assert(is_fixed_point());
         fixed_point_info get_fixed_point_info() const;
         
         // assert(is_floating_point());
         floating_point_info get_floating_point_info() const;
         
         // assert(is_integer() || is_enum() || is_fixed_point());
         integral_info get_integral_info() const;
         
         //#pragma region Arrays
            bool is_variable_length_array() const;
            
            // assert(is_array());
            // returns empty if a variable-length array.
            std::optional<size_t> array_extent() const;
            
            // returns 0 if not an array; 1 for foo[n]; 2 for foo[m][n]; etc.
            size_t array_rank() const;
            
            // assert(is_array());
            type array_value_type() const;
         //#pragma endregion
         
         //#pragma region Enums
            [[nodiscard]] std::vector<enum_member> all_enum_members() const;
         
            template<typename Functor>
            void for_each_enum_member(Functor&& functor) const;
         //#pragma endregion
         
         //#pragma region Functions
            // assert(is_function());
            type function_return_type() const;
            
            // assert(is_function());
            // Returns a list_node of raw arguments; the values are the argument types.
            // If this is not a varargs function, then the last element is the void type.
            list_node function_arguments() const;
            
            // assert(is_function());
            type nth_argument_type(size_t n) const;
            
            // assert(is_method());
            type is_method_of() const;
            
            bool is_varargs_function() const;
            
            // Does not include varargs; does include `this` for member functions.
            size_t fixed_function_arg_count() const;
            
            // weird C-only jank: `void f()` takes a variable number of arguments
            // (but isn't varargs??)
            bool is_unprototyped_function() const;
            
            // assert(is_function());
            template<typename Functor>
            void for_each_function_argument_type(Functor&& functor) const;
         //#pragma endregion
         
         //#pragma region Structs and unions
            // assert(is_record() || is_union());
            list_node all_members() const; // returns TREE_FIELDS(_node)
         
            template<typename Functor>
            void for_each_field(Functor&& functor) const;
            
            template<typename Functor>
            void for_each_static_data_member(Functor&& functor) const;
         //#pragma endregion
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"
#include "gcc_wrappers/type.inl"