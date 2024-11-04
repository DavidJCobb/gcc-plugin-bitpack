#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::type {
   WRAPPED_TREE_NODE_BOILERPLATE(container)
   
   list_node container::all_members() const {
      return list_node(TYPE_FIELDS(this->_node));
   }
}