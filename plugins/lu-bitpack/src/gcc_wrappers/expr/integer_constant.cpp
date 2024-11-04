#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(integer_constant)
   
   integer_constant::integer_constant(type::integral t, int n) {
      this->_node = build_int_cst(t.as_untyped(), n);
   }

   type::integral integer_constant::value_type() const {
      if (empty())
         return {};
      return type::integral::from_untyped(TREE_TYPE(this->_node));
   }
   
   std::optional<integer_constant::host_wide_int_type>  integer_constant::try_value_signed() const {
      if (empty())
         return {};
      assert(node_is(this->_node));
      if (!TREE_CONSTANT(this->_node))
         return {};
      
      if (!tree_fits_shwi_p(this->_node))
         return {};
      return (host_wide_int_type) TREE_INT_CST_LOW(this->_node);
   }
   std::optional<integer_constant::host_wide_uint_type> integer_constant::try_value_unsigned() const {
      if (empty())
         return {};
      assert(node_is(this->_node));
      if (!TREE_CONSTANT(this->_node))
         return {};
      
      if (!tree_fits_uhwi_p(this->_node))
         return {};
      return (host_wide_uint_type) TREE_INT_CST_LOW(this->_node);
   }
}