#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/identifier.h"
#include "gcc_wrappers/_node_boilerplate-impl.define.h"

#include <toplev.h> // rest_of_decl_compilation

// Defined in GCC source, `c/c-tree.h`; not included in the plug-in headers.
extern void c_bind(location_t, tree, bool);

namespace gcc_wrappers::decl {
   GCC_NODE_WRAPPER_BOILERPLATE(variable)
   
   variable::variable(
      lu::strings::zview identifier_name,
      type::base&        variable_type,
      location_t         source_location
   ) {
      auto identifier_node = identifier(identifier_name);
      this->_node = build_decl(
         source_location,
         VAR_DECL,
         identifier_node.unwrap(),
         variable_type.unwrap()
      );
   }
   variable::variable(
      identifier  identifier_node,
      type::base& variable_type,
      location_t  source_location
   ) {
      this->_node = build_decl(
         source_location,
         VAR_DECL,
         identifier_node.unwrap(),
         variable_type.unwrap()
      );
   }
   
   expr::declare variable::make_declare_expr() {
      return expr::declare(*this);
   }
   
   bool variable::is_defined_elsewhere() const {
      return DECL_EXTERNAL(this->_node);
   }
   void variable::set_is_defined_elsewhere(bool v) {
      DECL_EXTERNAL(this->_node) = v ? 1 : 0;
   }
   
   bool variable::is_externally_accessible() const {
      return TREE_PUBLIC(this->_node);
   }
   void variable::make_externally_accessible() {
      set_is_externally_accessible(true);
   }
   void variable::set_is_externally_accessible(bool v) {
      TREE_PUBLIC(this->_node) = v ? 1 : 0;
   }
   
   // https://gcc.gnu.org/pipermail/gcc/2022-November/240196.html
   void variable::make_file_scope_static() {
      TREE_STATIC(this->_node) = 1;
      c_bind(DECL_SOURCE_LOCATION(this->_node), this->_node, true); // c/c-tree.h
      rest_of_decl_compilation(this->_node, true, false); // toplev.h
   }
}