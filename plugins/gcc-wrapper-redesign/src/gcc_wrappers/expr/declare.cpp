#include "gcc_wrappers/expr/declare.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/_node_ref_boilerplate-impl.define.h"

#include <c-family/c-common.h> // build_stmt

namespace gcc_wrappers::expr {
   GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(declare)
   
   declare::declare(
      decl::variable& decl,
      location_t      source_location
   ) {
      this->_node = build_stmt(source_location, DECL_EXPR, decl.as_untyped());
   }
}