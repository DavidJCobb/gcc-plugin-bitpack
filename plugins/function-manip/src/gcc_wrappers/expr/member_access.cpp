#include "gcc_wrappers/expr/member_access.h"
#include "gcc_wrappers/decl/variable.h"
#include <plugin-version.h>
#include <c-tree.h> // build_component_ref
#include <stringpool.h> // get_identifier

namespace gcc_wrappers::decl {
   /*static*/ bool member_access::tree_is(tree node) {
      return TREE_CODE(node) == DECLARE_EXPR;
   }
   
   void member_access::set_from_untyped(tree src) {
      if (src != NULL_TREE) {
         assert(tree_is(src) && "expr::declare::set_from_untyped must be given a DECLARE_EXPR");
      }
      this->_node = src;
   }
   
   /*static*/ member_access member_access::create(
      decl::variable& object,
      const char*     field_name,
      location_t      access_location,
      location_t      field_name_location,
      location_t      arrow_operator_loc
   ) {
      auto field_identifier = get_identifier(field_name);
      switch (object.get_value_type().get_type_code()) {
         case type::type_code::RECORD_TYPE:
         case type::type_code::UNION_TYPE:
            break;
         default:
            assert(false && "cannot access member of something that isn't a class, struct, or union");
      }
      
      variable out;
      #if GCCPLUGIN_VERSION_MAJOR >= 9 && GCCPLUGIN_VERSION_MINOR >= 5
         #if GCCPLUGIN_VERSION_MAJOR >= 13 && GCCPLUGIN_VERSION_MINOR >= 3
            // https://github.com/gcc-mirror/gcc/commit/bb49b6e4f55891d0d8b596845118f40df6ae72a5
            out._node = build_component_ref(
               access_location,
               object.as_untyped(),
               field_identifier,
               field_name_location,
               arrow_operator_loc,
               true // default; todo: use false for offsetof, etc.?
            );
         #else
            // GCC PR91134
            // https://github.com/gcc-mirror/gcc/commit/7a3ee77a2e33b8b8ad31aea27996ebe92a5c8d83
            out._node = build_component_ref(
               access_location,
               object.as_untyped(),
               field_identifier,
               field_name_location,
               arrow_operator_loc
            );
         #endif
      #else
         out._node = build_component_ref(
            access_location,
            object.as_untyped(),
            field_identifier,
            field_name_location
         );
      #endif
      return out;
   }
   
   tree member_access::object_untyped() const {
      if (empty())
         return NULL_TREE;
      return TREE_OPERAND(this->_node, 0);
   }
   tree member_access::field_untyped() const {
      if (empty())
         return NULL_TREE;
      return TREE_OPERAND(this->_node, 1);
   }
}