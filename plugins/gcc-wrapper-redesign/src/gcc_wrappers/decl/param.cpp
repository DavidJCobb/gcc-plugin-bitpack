#include "gcc_wrappers/decl/param.h"
#include "gcc_wrappers/_node_ref_boilerplate-impl.define.h"

namespace gcc_wrappers::decl {
   GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(param)
   
   type::base param::effective_type() const {
      return type::base::wrap(DECL_ARG_TYPE(this->_node));
   }
}