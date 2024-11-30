#include "gcc_wrappers/decl/result.h"
#include "gcc_wrappers/_node_ref_boilerplate-impl.define.h"

namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(result)
   
   result::result(type::base t) {
      this->_node = build_decl(
         UNKNOWN_LOCATION,
         RESULT_DECL,
         NULL_TREE, // unnamed
         t.as_untyped()
      );
      this->make_artificial();
      this->make_sym_debugger_ignored();
   }
}