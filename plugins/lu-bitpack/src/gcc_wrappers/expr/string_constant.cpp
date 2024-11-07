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
}