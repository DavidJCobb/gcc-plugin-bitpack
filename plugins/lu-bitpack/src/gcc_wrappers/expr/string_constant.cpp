#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(string_constant)
   
   string_constant::string_constant(std::string_view s) {
      this->_node = build_string(s.size(), s.data());
   }
   
   std::string string_constant::value() const {
      return std::string(value_view());
   }
   std::string_view string_constant::value_view() const {
      if (empty())
         return {};
      assert(node_is(this->_node));
      if (!TREE_CONSTANT(this->_node))
         return {};
      
      auto size_of = TREE_STRING_LENGTH(this->_node); // includes null
      if (size_of == 0)
         return {};
      return std::string_view(TREE_STRING_POINTER(this->_node), size_of - 1);
   }
   
   size_t string_constant::length() const {
      return TREE_STRING_LENGTH(this->_node);
   }
   
   bool string_constant::referenced_by_string_literal() const {
      //
      // `build_string` doesn't set `TREE_TYPE` when making a string.
      // `build_string_literal` creates a string and then sets the 
      // `TREE_TYPE` and other properties on it.
      //
      return TREE_TYPE(this->_node) != NULL_TREE;
   }
   
   value string_constant::to_string_literal(type::base character_type) {
      assert(!empty());
      if (character_type.empty()) {
         character_type.set_from_untyped(char_type_node);
         assert(!character_type.empty());
      }
      {
         auto prior = TREE_TYPE(this->_node);
         if (prior != NULL_TREE && prior != character_type.as_untyped()) {
            return string_constant(this->value_view()).to_string_literal(character_type);
         }
      }
      
      assert(!referenced_by_string_literal());
      
      //
      // Based on `build_string_literal`:
      //
      
      tree index  = build_index_type(size_int(this->length()));
      tree e_type = build_type_variant(character_type.as_untyped(), 1, 0);
      tree a_type = build_array_type(e_type, index);
      
      TREE_TYPE(this->_node)     = a_type;
      TREE_CONSTANT(this->_node) = 1;
      TREE_READONLY(this->_node) = 1;
      TREE_STATIC(this->_node)   = 1;
      
      tree l_type = build_pointer_type(e_type);
      
      // &char_array[0]
      tree literal = build1(
         ADDR_EXPR,
         l_type,
         build4(
            ARRAY_REF,
            e_type,
            this->_node,
            integer_zero_node,
            NULL_TREE,
            NULL_TREE
         )
      );
      
      ::gcc_wrappers::value v;
      v.set_from_untyped(literal);
      return v;
   }
}