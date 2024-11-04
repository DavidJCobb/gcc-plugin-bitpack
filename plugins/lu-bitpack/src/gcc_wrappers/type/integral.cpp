#include "gcc_wrappers/type/integral.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::type {
   WRAPPED_TREE_NODE_BOILERPLATE(integral)
   
   size_t integral::bitcount() const {
      return TYPE_PRECISION(this->_node);
   }
   
   bool integral::is_signed() const {
      return !is_unsigned();
   }
   bool integral::is_unsigned() const {
      return TYPE_UNSIGNED(this->_node) != 0;
   }
   
   intmax_t integral::minimum_value() const {
      auto data = TYPE_MIN_VALUE(this->_node);
      if (TREE_CODE(data) == INTEGER_CST)
         return TREE_INT_CST_LOW(data);
      return 0;
   }
   uintmax_t integral::maximum_value() const {
      auto data = TYPE_MAX_VALUE(this->_node);
      if (TREE_CODE(data) == INTEGER_CST)
         return TREE_INT_CST_LOW(data);
      return 0;
   }
   
   integral integral::make_signed() const {
      return integral::from_untyped(signed_type_for(this->_node)); // tree.h
   }
   integral integral::make_unsigned() const {
      return integral::from_untyped(unsigned_type_for(this->_node)); // tree.h
   }
}