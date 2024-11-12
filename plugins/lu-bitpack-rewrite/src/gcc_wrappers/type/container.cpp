#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::type {
   WRAPPED_TREE_NODE_BOILERPLATE(container)
   
   chain_node container::all_members() const {
      return chain_node(TYPE_FIELDS(this->_node));
   }
}