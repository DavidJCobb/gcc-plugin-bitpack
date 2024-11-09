#pragma once
#include <string>
#include "gcc_wrappers/_wrapped_tree_node.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class type_def;
   }
   namespace type {
      class array;
      class enumeration;
      class fixed_point;
      class floating_point;
      class function;
      class integral;
      class pointer;
      class record;
      class untagged_union;
   }
   class attribute_list;
}

namespace gcc_wrappers::type {
   class base : public _wrapped_tree_node {
      public:
         static bool node_is(tree t) {
            return t != NULL_TREE && TYPE_P(t);
         }
         WRAPPED_TREE_NODE_BOILERPLATE(base)
         
      public:
         base() {}
         
         bool operator==(const base) const;
         
         std::string name() const;
         std::string pretty_print() const;
         
         // NOTE: This only includes attributes that GCC has decided to apply directly 
         // to the type (generally done when it's a struct or union). It notably does 
         // not include attributes applied to a `typedef`; those exist on the TYPE_DECL, 
         // not on the *_TYPE that the TYPE_DECL creates.
         attribute_list attributes();
         const attribute_list attributes() const;
         
         base canonical() const;
         base main_variant() const;
         
         // If this type was defined via the `typedef` keyword, returns the TYPE_DECL 
         // produced by that keyword. You can walk transitive typedefs this way. If 
         // this type isn't the result of a `typedef`, then the result is `empty()`.
         //
         // Make sure to include the header for `type_def` yourself before calling. 
         // Can't include it for you here or we'll get a circular dependency.
         decl::type_def declaration() const;
         
         bool is_type_or_transitive_typedef_thereof(base) const;
         
         bool has_user_requested_alignment() const; // __attribute__((align...))
         bool is_complete() const;
         bool is_packed() const;
         
         size_t size_in_bits() const;
         size_t size_in_bytes() const;
         
         //
         // Type checks:
         //
         
         bool is_arithmetic() const; // INTEGER_TYPE or REAL_TYPE
         bool is_array() const;
         bool is_boolean() const;
         bool is_complex() const; // COMPLEX_TYPE
         bool is_enum() const; // ENUMERAL_TYPE
         bool is_fixed_point() const;
         bool is_floating_point() const; // REAL_TYPE
         bool is_function() const; // FUNCTION_TYPE or METHOD_TYPE
         bool is_integer() const; // does not include enums or bools
         bool is_method() const; // METHOD_TYPE
         bool is_record() const; // class or struct
         bool is_union() const;
         bool is_void() const;
         
         //
         // Casts:
         //
         
         array as_array();
         const array as_array() const;
         
         enumeration as_enum();
         const enumeration as_enum() const;
         
         floating_point as_floating_point();
         const floating_point as_floating_point() const;
         
         fixed_point as_fixed_point();
         const fixed_point as_fixed_point() const;
         
         function as_function();
         const function as_function() const;
         
         integral as_integral();
         const integral as_integral() const;
         
         pointer as_pointer();
         const pointer as_pointer() const;
         
         record as_record();
         const record as_record() const;
         
         untagged_union as_union();
         const untagged_union as_union() const;
         
         //
         // Queries and transformations:
         //
         
         base add_const() const;
         base remove_const() const;
         bool is_const() const;
         
         bool is_volatile() const;
         
         pointer add_pointer() const;
         base remove_pointer() const;
         bool is_pointer() const;
         
         array add_array_extent(size_t) const;
         
         // assert(!is_packed())
         // If this type already has the desired alignment (whether user-requested 
         // or not), returns self.
         base with_user_defined_alignment(size_t bytes);
         base with_user_defined_alignment_in_bits(size_t bits);
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"