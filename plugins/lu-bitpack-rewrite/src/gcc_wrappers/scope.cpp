#include "gcc_wrappers/scope.h"
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/type/base.h"

#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers {
   WRAPPED_TREE_NODE_BOILERPLATE(scope)
   
   /*static*/ bool scope::node_is(tree t) {
      if (t == NULL_TREE)
         return false;
      
      // Based on the switch-case in `decl_type_context` in `tree.cc`:
      switch (TREE_CODE(t)) {
         case TRANSLATION_UNIT_DECL:
         case NAMESPACE_DECL:
         
         case TYPE_DECL:
         
         // Types and declarations can be scoped to a block or a function; 
         // blocks themselves are scoped to functions.
         case FUNCTION_DECL:
         case BLOCK:
         
         // The only types that can contain other types or declarations.
         case RECORD_TYPE:
         case UNION_TYPE:
         case QUAL_UNION_TYPE:
            return true;
            
         default:
            break;
      }
      
      return false;
   }
   
   scope scope::containing_scope() {
      assert(!empty());
      return from_untyped(::get_containing_scope(this->_node));
   }
   
   bool scope::is_declaration() const {
      return !empty() && DECL_P(this->_node);
   }
   bool scope::is_file_scope() const {
      return !empty() && TREE_CODE(this->_node) == TRANSLATION_UNIT_DECL;
   }
   bool scope::is_namespace() const {
      return !empty() && TREE_CODE(this->_node) == NAMESPACE_DECL;
   }
   bool scope::is_type() const {
      return !empty() && TYPE_P(this->_node);
   }
   
   decl::base scope::as_declaration() {
      assert(is_declaration());
      return decl::base::from_untyped(this->_node);
   }
   type::base scope::as_type() {
      assert(is_type());
      return type::base::from_untyped(this->_node);
   }
         
   decl::function scope::nearest_function() {
      if (empty())
         return {};
      
      // Can't use this; it assumes the argument is a DECL.
      //return decl::function::from_untyped(decl_function_context(this->_node));
      
      scope current = *this;
      while (!current.empty() && current.code() != FUNCTION_DECL) {
         current = current.containing_scope();
      }
      return decl::function::from_untyped(current.as_untyped());
   }
   
   type::base scope::nearest_type() {
      if (empty())
         return {};
      
      scope current = *this;
      while (!current.empty()) {
         switch (current.code()) {
            case TRANSLATION_UNIT_DECL:
            case NAMESPACE_DECL:
               return {};
            case RECORD_TYPE:
            case UNION_TYPE:
            case QUAL_UNION_TYPE:
               return type::base::from_untyped(current.as_untyped());
            case TYPE_DECL:
            case FUNCTION_DECL:
            case BLOCK:
               break;
            default:
               assert(false && "unreachable");
         }
         current = current.containing_scope();
      }
      return {};
   }
}