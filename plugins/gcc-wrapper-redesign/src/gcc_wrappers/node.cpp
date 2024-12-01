#include "gcc_wrappers/node.h"
#include <cassert>

namespace gcc_wrappers {
   /*static*/ node node::wrap(tree n) {
      assert(n != NULL_TREE);
      node out;
      out._node = n;
      return out;
   }
   
   tree_code node::code() const {
      return TREE_CODE(this->_node);
   }
   const std::string_view node::code_name() const noexcept {
      return get_tree_code_name(TREE_CODE(this->_node));
   }
   
   const_tree node::unwrap() const {
      return this->_node;
   }
   tree node::unwrap()  {
      return this->_node;
   }
}