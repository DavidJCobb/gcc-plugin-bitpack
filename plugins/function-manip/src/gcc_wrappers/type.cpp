#include "gcc_wrappers/type.h"
#include <cassert>

#include <c-family/c-common.h> // c_build_qualified_type; various predefined type nodes

namespace gcc_wrappers {
   /*static*/ bool type::tree_is(tree node) {
      return TYPE_P(node);
   }
   
   type type::add_const_qualifier() const {
      type out;
      
      auto altered = c_build_qualified_type(this->_type, TYPE_QUAL_CONST);
      out.set_from_untyped(altered);
      return out;
   }
   type type::add_pointer() const {
      type out;
      
      auto altered = build_pointer_type(this->_type);
      out.set_from_untyped(altered);
      return out;
   }
   
   type_code type::get_type_code() const {
      if (this->_type == NULL_TREE)
         return type_code::NONE;
      return TREE_CODE(this->_type);
   }
   
   void type::set_from_untyped(tree src) {
      if (src != NULL_TREE) {
         assert(tree_is(src) && "type::set_from_untyped must be given a type");
      }
      this->_type = src;
   }
}