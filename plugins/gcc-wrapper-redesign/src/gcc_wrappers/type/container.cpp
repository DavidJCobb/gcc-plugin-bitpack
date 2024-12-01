#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/_node_boilerplate-impl.define.h"

namespace gcc_wrappers::type {
   GCC_NODE_WRAPPER_BOILERPLATE(container)
   
   chain container::member_chain() const {
      return chain(TYPE_FIELDS(this->_node));
   }
}