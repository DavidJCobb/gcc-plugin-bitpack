#include "gcc_wrappers/identifier.h"
#include <stringpool.h> // get_identifier
#include "gcc_wrappers/_node_boilerplate-impl.define.h"

namespace gcc_wrappers {
   GCC_NODE_WRAPPER_BOILERPLATE(identifier)
   
   /*static*/ bool identifier::raw_node_is(tree t) {
      return TREE_CODE(t) == IDENTIFIER_NODE;
   }
   
   identifier::identifier(lu::strings::zview name) {
      this->_node = get_identifier(name.c_str());
   }
   
   std::string_view identifier::name() const {
      return std::string_view(IDENTIFIER_POINTER(this->_node));
   }
}