#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"
#include <cassert>
#include <c-family/c-common.h> // c_build_qualified_type, c_type_promotes_to

// Defined in GCC source, `c-tree.h`; not included in the plug-in headers, bafflingly.
// Do not confuse this with the C++ `comptypes`, which has a different signature and 
// is unavailable when compiling C.
extern int comptypes(tree, tree);

// For casts:
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/type/enumeration.h"
#include "gcc_wrappers/type/fixed_point.h"
#include "gcc_wrappers/type/floating_point.h"
#include "gcc_wrappers/type/function.h"
#include "gcc_wrappers/type/integral.h"
#include "gcc_wrappers/type/pointer.h"
#include "gcc_wrappers/type/record.h"
#include "gcc_wrappers/type/union.h"

namespace gcc_wrappers::type {
   WRAPPED_TREE_NODE_BOILERPLATE(base)
   
   bool base::operator==(const type other) const {
      if (this->_node == other._node)
         return true;
      
      gcc_assert(c_language == c_language_kind::clk_c);
      return comptypes(this->_node, other._node) != 0;
   }
   
   std::string base::name() const {
      tree name_tree = TYPE_NAME(this->_node);
      if (!name_tree) {
         return {}; // unnamed struct
      }
      if (TREE_CODE(name_tree) == IDENTIFIER_NODE)
         return IDENTIFIER_POINTER(name_tree);
      if (TREE_CODE(name_tree) == TYPE_DECL && DECL_NAME(name_tree))
         return IDENTIFIER_POINTER(DECL_NAME(name_tree));
      //
      // Name irretrievable?
      //
      return {};
   }
   std::string base::pretty_print() const {
      if (empty())
         return "<empty>";
      
      std::string out;
      
      //
      // Try printing function types and function pointer types.
      //
      {
         bool is_const_pointer    = false;
         bool is_function_pointer = false;
         type ft;
         if (is_function()) {
            ft = *this;
         } else if (is_pointer()) {
            auto pt = remove_pointer();
            if (pt.is_function()) {
               ft = pt;
               is_function_pointer = true;
               is_const_pointer    = ft.is_const();
            }
         }
         if (!ft.empty()) {
            out = ft.function_return_type().pretty_print();
            if (is_function_pointer) {
               out += "(*";
               if (is_const_pointer) {
                  out += " const";
               }
               out += ')';
            } else {
               out += "(&)"; // not sure if this is correct
            }
            out += '(';
            if (!ft.is_unprototyped_function()) {
               bool first   = true;
               bool varargs = true;
               auto args    = ft.function_arguments();
               args.for_each_value([&first, &out, &varargs](tree raw) {
                  if (raw == void_type_node) {
                     varargs = false;
                     return;
                  }
                  type t = base::from_untyped(raw);
                  if (first) {
                     first = false;
                  } else {
                     out += ", ";
                  }
                  out += t.pretty_print();
               });
            }
            out += ')';
            return out;
         }
      }
      
      //
      // Try printing array types.
      //
      if (is_array()) {
         out = array_value_type().pretty_print();
         out += '[';
         if (auto extent = array_extent(); extent.has_value())
            out += *extent; // TODO: stringify number
         out += ']';
         return out;
      }
      
      //
      // Try printing pointer types.
      //
      if (is_pointer()) {
         out += remove_pointer().pretty_print();
         out += '*';
         if (is_const())
            out += " const";
         return out;
      }
      
      //
      // Base case.
      //
      
      if (is_const())
         out = "const ";
      
      auto id = TYPE_NAME(this->_node);
      switch (TREE_CODE(id)) {
         case IDENTIFIER_NODE:
            out += IDENTIFIER_POINTER(id);
            break;
         case TYPE_DECL:
            out += IDENTIFIER_POINTER(DECL_NAME(id));
            break;
         default:
            out += "<unnamed>";
            break;
      }
      
      return out;
   }
   
   list_node base::attributes() const {
      return list_node(TYPE_ATTRIBUTES(this->_node));
   }
   
   base base::canonical() const {
      type out;
      out.set_from_untyped(TYPE_CANONICAL(this->_node));
      return out;
   }
   base base::main_variant() const {
      type out;
      out.set_from_untyped(TYPE_MAIN_VARIANT(this->_node));
      return out;
   }
   
   decl::type_def base::declaration() const {
      auto dn = TYPE_NAME(this->_node);
      if (dn != NULL_TREE) {
         if (TREE_CODE(dn) != TYPE_DECL)
            dn = NULL_TREE;
      }
      return decl::type_def::from_untyped(dn);
   }
   
   bool base::is_complete() const {
      return COMPLETE_TYPE_P(this->_node);
   }
   
   size_t base::size_in_bits() const {
      auto size_node = TYPE_SIZE(this->_node);
      if (TREE_CODE(size_node) != INTEGER_CST)
         return 0;
      
      // This truncates, but if you have a type that's larger than 
      // (std::numeric_limits<uint32_t>::max() / 8), then you're 
      // probably screwed anyway tbh
      return TREE_INT_CST_LOW(size_node);
   }
   size_t base::size_in_bytes() const {
      return this->size_in_bits() / 8;
   }
   
   bool base::is_arithmetic() const {
      auto code = TREE_CODE(this->_node);
      return code == INTEGER_TYPE || code == REAL_TYPE;
   }
   bool base::is_array() const {
      return TREE_CODE(this->_node) == ARRAY_TYPE;
   }
   bool base::is_boolean() const {
      return TREE_CODE(this->_node) == BOOLEAN_TYPE;
   }
   bool base::is_enum() const {
      return TREE_CODE(this->_node) == ENUMERAL_TYPE;
   }
   bool base::is_fixed_point() const {
      return TREE_CODE(this->_node) == FIXED_POINT_TYPE;
   }
   bool base::is_floating_point() const {
      return TREE_CODE(this->_node) == REAL_TYPE;
   }
   bool base::is_function() const {
      auto code = TREE_CODE(this->_node);
      return code == FUNCTION_TYPE || code == METHOD_TYPE;
   }
   bool base::is_integer() const {
      return TREE_CODE(this->_node) == INTEGER_TYPE;
   }
   bool base::is_method() const {
      return TREE_CODE(this->_node) == METHOD_TYPE;
   }
   bool base::is_record() const {
      return TREE_CODE(this->_node) == RECORD_TYPE;
   }
   bool base::is_union() const {
      return TREE_CODE(this->_node) == UNION_TYPE;
   }
   bool base::is_void() const {
      return TREE_CODE(this->_node) == VOID_TYPE;
   }
   
   //
   // Casts:
   //
   
   array base::as_array() {
      return const_cast<array>(std::as_const(*this).as_array());
   }
   const array base::as_array() const {
      assert(is_array());
      return array::from_untyped(this->_node);
   }
   
   enumeration base::as_enum() {
      return const_cast<enumeration>(std::as_const(*this).as_enum());
   }
   const enumeration base::as_enum() const {
      assert(is_enum());
      return enumeration::from_untyped(this->_node);
   }
   
   floating_point base::as_floating_point() {
      return const_cast<floating_point>(std::as_const(*this).as_floating_point());
   }
   const floating_point base::as_floating_point() const {
      assert(is_floating_point());
      return floating_point::from_untyped(this->_node);
   }
   
   fixed_point base::as_fixed_point() {
      return const_cast<fixed_point>(std::as_const(*this).as_fixed_point());
   }
   const fixed_point base::as_fixed_point() const {
      assert(is_fixed_point());
      return fixed_point::from_untyped(this->_node);
   }
         
   function base::as_function() {
      return const_cast<function>(std::as_const(*this).as_function());
   }
   const function base::as_function() const {
      assert(is_function());
      return function::from_untyped(this->_node);
   }
   
   integral base::as_integral() {
      return const_cast<integral>(std::as_const(*this).as_integral());
   }
   const integral base::as_integral() const {
      assert(is_integral());
      return integral::from_untyped(this->_node);
   }
   
   pointer base::as_pointer() {
      return const_cast<pointer>(std::as_const(*this).as_pointer());
   }
   const pointer base::as_pointer() const {
      assert(is_pointer());
      return pointer::from_untyped(this->_node);
   }
   
   record base::as_record() {
      return const_cast<record>(std::as_const(*this).as_record());
   }
   const record base::as_record() const {
      assert(is_record());
      return record::from_untyped(this->_node);
   }
   
   untagged_union base::as_union() {
      return const_cast<untagged_union>(std::as_const(*this).as_union());
   }
   const untagged_union base::as_union() const {
      assert(is_union());
      return untagged_union::from_untyped(this->_node);
   }
   
   //
   // Queries and transformations:
   //
   
   base base::add_const() const {
      gcc_assert(!empty());
      return base::from_untyped(c_build_qualified_type(this->_node, TYPE_QUAL_CONST));
   }
   base base::remove_const() const {
      if (!is_const())
         return *this;
      return base::from_untyped(c_build_qualified_type(this->_node, 0));
   }
   bool base::is_const() const {
      return !empty() && TREE_READONLY(this->_node);
   }
   
   bool base::is_volatile() const {
      return !empty() && TYPE_VOLATILE(this->_node);
   }
   
   base base::add_pointer() const {
      gcc_assert(!empty());
      return base::from_untyped(build_pointer_type(this->_node));
   }
   base base::remove_pointer() const {
      if (!is_pointer())
         return *this;
      return base::from_untyped(TREE_TYPE(this->_node));
   }
   bool base::is_pointer() const {
      return !empty() && TREE_CODE(this->_node) == POINTER_TYPE;
   }
}