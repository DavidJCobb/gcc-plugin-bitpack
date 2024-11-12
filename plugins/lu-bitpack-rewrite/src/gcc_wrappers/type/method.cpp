#include "gcc_wrappers/type/method.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::type {
   WRAPPED_TREE_NODE_BOILERPLATE(method)
   
   base method::is_method_of() const {
      assert(!empty());
      return base::from_untyped(TYPE_METHOD_BASETYPE(this->_node));
   }
}