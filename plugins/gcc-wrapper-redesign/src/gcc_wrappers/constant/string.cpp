#include "gcc_wrappers/constant/string.h"
#include "gcc_wrappers/_node_boilerplate-impl.define.h"

namespace gcc_wrappers::constant {
   GCC_NODE_WRAPPER_BOILERPLATE(string)
   
   string::string(const std::string& s) {
      this->_node = build_string(s.size() + sizeof('\0'), s.c_str());
   }
   string::string(lu::strings::zview view) {
      this->_node = build_string(view.size() + sizeof('\0'), view.c_str());
   }
   string::string(const char* s, size_t size_incl_null) {
      assert(s[size_incl_null] == '\0');
      this->_node = build_string(size_incl_null, s);
   }
   
   std::string_view string::value() const {
      return std::string_view(TREE_STRING_POINTER(this->_node), this->length());
   }
   
   size_t string::length() const {
      auto v = this->terminated_length();
      assert(v > 0);
      --v;
      return v;
   }
   size_t string::terminated_length() const {
      return TREE_STRING_LENGTH(this->_node);
   }
   
   bool string::referenced_by_string_literal() const {
      //
      // `build_string` doesn't set `TREE_TYPE` when making a string.
      // `build_string_literal` creates a string and then sets the 
      // `TREE_TYPE` and other properties on it.
      //
      return TREE_TYPE(this->_node) != NULL_TREE;
   }
   
   value string::to_string_literal(type::base character_type) {
      if (character_type.empty()) {
         character_type.set_from_untyped(char_type_node);
         assert(!character_type.empty());
      }
      {
         auto prior = TREE_TYPE(this->_node);
         if (prior != NULL_TREE && prior != character_type.as_untyped()) {
            auto data  = this->value_view();
            auto clone = string(data.data(), data.size() + 1);
            return clone.to_string_literal(character_type);
         }
      }
      
      assert(!referenced_by_string_literal());
      
      //
      // Based on `build_string_literal`:
      //
      
      tree index  = build_index_type(size_int(this->terminated_length()));
      tree e_type = build_type_variant(character_type.as_untyped(), 1, 0);
      tree a_type = build_array_type(e_type, index);
      
      TREE_TYPE(this->_node)     = a_type;
      TREE_CONSTANT(this->_node) = 1;
      TREE_READONLY(this->_node) = 1;
      TREE_STATIC(this->_node)   = 1;
      
      tree l_type = build_pointer_type(e_type);
      
      // &__this_char_array[0]
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
      
      return value::wrap(literal);
   }
}