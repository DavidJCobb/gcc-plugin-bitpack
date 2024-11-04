#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::type {
   WRAPPED_TREE_NODE_BOILERPLATE(array)
   
   bool array::is_variable_length_array() const {
      return this->extent().has_value();
   }
   
   std::optional<size_t> array::extent() const {
      assert(!empty());
      auto domain = TYPE_DOMAIN(this->_node);
      if (domain == NULL_TREE)
         return {};
      auto max = TYPE_MAX_VALUE(domain);
      //
      // Fail on non-constant:
      //
      if (!max)
         return {};
      if (TREE_CODE(max) != INTEGER_CST)
         return {};
      if (!TREE_CONSTANT(max))
         return {};
      //
      return (size_t)TREE_INT_CST_LOW(max) + 1;
   }
   
   size_t array::rank() const {
      assert(!empty());
      size_t i = 0;
      auto   t = this->_node;
      do {
         ++i;
      } while (t = TREE_TYPE(t), t != NULL_TREE && TREE_CODE(t) == ARRAY_TYPE);
      return i;
   }
   
   base array::value_type() const {
      assert(!empty());
      return base::from_untyped(TREE_TYPE(this->_node));
   }
}