#include "gcc_wrappers/type/floating_point.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::type {
   WRAPPED_TREE_NODE_BOILERPLATE(floating_point)
   
   size_t floating_point::bitcount() const {
      return TYPE_PRECISION(this->_node);
   }
   
   bool floating_point::is_single_precision() const {
      return bitcount() == 32;
   }
   bool floating_point::is_double_precision() const {
      return bitcount() == 64;
   }
}