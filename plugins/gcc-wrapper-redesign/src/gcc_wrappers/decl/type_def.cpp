#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/_node_ref_boilerplate-impl.define.h"

namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(type_def)
   
   type::base_ptr type_def::declared() const {
      return type::base_ptr(TREE_TYPE(this->_node));
   }
   type::base_ptr type_def::is_synonym_of() const {
      auto orig = DECL_ORIGINAL_TYPE(this->_node);
      return type::base_ptr(orig);
   }
}