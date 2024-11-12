#include "gcc_wrappers/decl/param.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(param)
   
   type::base param::value_type() const {
      if (empty())
         return {};
      return type::base::from_untyped(TREE_TYPE(this->_node));
   }
   type::base param::effective_type() const {
      if (empty())
         return {};
      return type::base::from_untyped(DECL_ARG_TYPE(this->_node));
   }
}