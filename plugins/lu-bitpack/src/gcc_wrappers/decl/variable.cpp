#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

#include <stringpool.h> // get_identifier
#include <toplev.h> // rest_of_decl_compilation

// Defined in GCC source, `c/c-tree.h`; not included in the plug-in headers.
extern void c_bind(location_t, tree, bool);

namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(variable)
   
   variable::variable(
      const char*       identifier_name,
      const type::base& variable_type,
      location_t        source_location
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
   
   type::base variable::value_type() const {
      if (empty())
         return {};
      return type::base::from_untyped(TREE_TYPE(this->_node));
   }
   
   value variable::initial_value() const {
      assert(!empty());
      auto node = DECL_INITIAL(this->_node);
      value v;
      v.set_from_untyped(node);
      return v;
   }
   void variable::set_initial_value(value v) {
      assert(!empty());
      DECL_INITIAL(this->_node) = v.as_untyped();
   }
   
   bool variable::is_defined_elsewhere() const {
      if (empty())
         return false;
      return DECL_EXTERNAL(this->_node);
   }
   void variable::set_is_defined_elsewhere(bool v) {
      assert(!empty());
      DECL_EXTERNAL(this->_node) = v ? 1 : 0;
   }
   
   bool variable::is_externally_accessible() const {
      if (empty())
         return false;
      return TREE_PUBLIC(this->_node);
   }
   void variable::make_externally_accessible() {
      set_is_externally_accessible(true);
   }
   void variable::set_is_externally_accessible(bool v) {
      assert(!empty());
      TREE_PUBLIC(this->_node) = v ? 1 : 0;
   }
   
   // https://gcc.gnu.org/pipermail/gcc/2022-November/240196.html
   void variable::make_file_scope_static() {
      assert(!empty());
      TREE_STATIC(this->_node) = 1;
      c_bind(DECL_SOURCE_LOCATION(this->_node), this->_node, true); // c/c-tree.h
      rest_of_decl_compilation(this->_node, true, false); // toplev.h
   }
}