#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(type_def)
   
   type type_def::declared() const {
      if (empty())
         return {};
      
      return type::from_untyped(TREE_TYPE(this->_node));
   }
   type type_def::is_synonym_of() const {
      if (empty())
         return {};
      
      auto orig = DECL_ORIGINAL_TYPE(this->_node);
      if (orig == NULL_TREE || !TYPE_P(orig))
         return {};
      
      return type::from_untyped(orig);
   }
}