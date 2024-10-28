#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

#include <stringpool.h> // get_identifier

namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(variable)
   
   variable::variable(
      const char* identifier_name,
      const type& variable_type,
      location_t  source_location
   ) {
      this->_node = build_decl(
         source_location,
         VAR_DECL,
         get_identifier(identifier_name),
         variable_type.as_untyped()
      );
   }
   
   expr::declare variable::make_declare_expr() {
      return expr::declare(*this);
   }
   
   type variable::value_type() const {
      if (empty())
         return {};
      type t;
      t.set_from_untyped(TREE_TYPE(this->_node));
      return t;
   }
}