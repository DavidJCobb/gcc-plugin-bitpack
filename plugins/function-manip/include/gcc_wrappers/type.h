#pragma once
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_wrappers {
   class type {
      protected:
         tree _type = NULL_TREE;
      
      public:
         enum class type_code {
            NONE = -1,
            
            ARRAY_TYPE       = ::ARRAY_TYPE,
            BITINT_TYPE      = ::BITINT_TYPE, // _BitInt(N)
            BOOLEAN_TYPE     = ::BOOLEAN_TYPE,
            COMPLEX_TYPE     = ::COMPLEX_TYPE,
            ENUMERAL_TYPE    = ::ENUMERAL_TYPE, // enumeration
            FUNCTION_TYPE    = ::FUNCTION_TYPE, // non-member/static-member functions' types
            FIXED_POINT_TYPE = ::FIXED_POINT_TYPE, // _Fract and friends
            INTEGER_TYPE     = ::INTEGER_TYPE, // does not include enums or bools
            LANG_TYPE        = ::LANG_TYPE,
            METHOD_TYPE      = ::METHOD_TYPE, // non-static member functions' types
            OFFSET_TYPE      = ::OFFSET_TYPE,
            OPAQUE_TYPE      = ::OPAQUE_TYPE,
            POINTER_TYPE     = ::POINTER_TYPE, // pointers and pointers-to-member
            QUAL_UNION_TYPE  = ::QUAL_UNION_TYPE, // Ada-specific; not relevant to us
            REAL_TYPE        = ::REAL_TYPE, // float, double, long double
            RECORD_TYPE      = ::RECORD_TYPE, // class, struct, pointer-to-member-function, etc.
            REFERENCE_TYPE   = ::REFERENCE_TYPE,
            UNION_TYPE       = ::UNION_TYPE,
            VOID_TYPE        = ::VOID_TYPE,
         };
      
      public:
         type() {}
         
         static bool tree_is(tree);
         
         constexpr const tree& as_untyped() const {
            return this->_type;
         }
         constexpr tree& as_untyped() {
            return this->_type;
         }
         
         type add_const_qualifier() const;
         type add_pointer() const;
         
         type_code get_type_code() const;
         
         void set_from_untyped(tree);
   };
}