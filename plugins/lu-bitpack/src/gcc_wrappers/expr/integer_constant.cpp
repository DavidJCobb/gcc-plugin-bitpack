#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(integer_constant)
   
   /*explicit*/ integer_constant::integer_constant(type::integral t, int n) {
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
   
   int integer_constant::sign() const {
      return tree_int_cst_sgn(this->_node);
   }
            
   bool integer_constant::operator<(const integer_constant& other) const {
      //
      // NOTE: This doesn't handle the case of the values having types of 
      // different signedness.
      //
      return tree_int_cst_lt(this->_node, other._node);
   }
   bool integer_constant::operator==(const integer_constant& other) const {
      return tree_int_cst_equal(this->_node, other._node);
   }
   
   bool integer_constant::operator==(host_wide_uint_type v) const {
      return compare_tree_int(this->_node, v);
   }
}