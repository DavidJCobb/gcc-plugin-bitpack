#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"
#include <cassert>

namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(base)
   
   std::string_view base::name() const {
      if (empty())
         return {};
      return std::string_view(IDENTIFIER_POINTER(DECL_NAME(this->_node)));
   }
   
   location_t base::source_location() const {
      return DECL_SOURCE_LOCATION(this->_node);
   }
   
   std::string_view base::source_file() const {
      if (empty())
         return {};
      return std::string_view(DECL_SOURCE_FILE(this->_node));
   }
   int base::source_line() const {
      if (empty())
         return 0;
      return DECL_SOURCE_LINE(this->_node);
   }
   
   bool base::linkage_status_unknown() const {
      if (empty())
         return false;
      switch (TREE_CODE(this->_node)) {
         case FUNCTION_DECL:
         case TYPE_DECL:
         case VAR_DECL:
            break;
         default:
            return false;
      }
      return DECL_DEFER_OUTPUT(this->_node);
   }
   
   bool base::is_artificial() const {
      return DECL_ARTIFICIAL(this->_node) != 0;
   }
   void base::make_artificial() {
      set_is_artificial(true);
   }
   void base::set_is_artificial(bool v) {
      assert(!empty());
      DECL_ARTIFICIAL(this->_node) = v;
   }
            
   bool base::is_sym_debugger_ignored() const {
      return DECL_IGNORED_P(this->_node) != 0;
   }
   void base::make_sym_debugger_ignored() {
      set_is_sym_debugger_ignored(true);
   }
   void base::set_is_sym_debugger_ignored(bool v) {
      assert(!empty());
      DECL_IGNORED_P(this->_node) = v;
   }
   
   bool base::is_used() const {
      return TREE_USED(this->_node) != 0;
   }
   void base::make_used() {
      set_is_used(true);
   }
   void base::set_is_used(bool v) {
      assert(!empty());
      TREE_USED(this->_node) = v;
   }
   
   // TODO: what types can this be?
   _wrapped_tree_node base::context() const {
      _wrapped_tree_node w;
      w.set_from_untyped(DECL_CONTEXT(this->_node));
      return w;
   }
}