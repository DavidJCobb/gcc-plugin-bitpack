#include "gcc_wrappers/type/function.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::type {
   WRAPPED_TREE_NODE_BOILERPLATE(function)
   
   base function::return_type() const {
      assert(!empty());
      return base::from_untyped(TREE_TYPE(this->_node));
   }
   
   list_node function::arguments() const {
      assert(!empty());
      return list_node(TYPE_ARG_TYPES(this->_node));
   }
   
   base function::nth_argument_type(size_t n) const {
      assert(!empty());
      return base::from_untyped(function_arguments().untyped_nth_value(n));
   }
   
   bool function::is_varargs() const {
      if (empty())
         return false;
      
      auto list = this->function_arguments();
      if (list.empty())
         return false; // unprototyped
      
      auto back = list.untyped_back();
      if (back != NULL_TREE && TREE_VALUE(back) == void_list_node)
         return true;
      
      return false;
   }
   
   size_t function::fixed_argument_count() const {
      assert(!empty());
      auto   list = function_arguments();
      size_t size = list.size();
      if (size) {
         if (list.untyped_back() == void_list_node)
            --size;
      }
      return size;
   }
   
   bool function::is_unprototyped() const {
      return !empty() && TYPE_ARG_TYPES(this->_node) == NULL;
   }
}