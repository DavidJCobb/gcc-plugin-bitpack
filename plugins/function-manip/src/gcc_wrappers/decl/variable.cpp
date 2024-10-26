#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/type.h"

#include <stringpool.h> // get_identifier

namespace gcc_wrappers::decl {
   /*static*/ bool base::tree_is(tree node) {
      return TREE_CODE(node) == VAR_DECL;
   }
   
   void variable::set_from_untyped(tree src) {
      if (src != NULL_TREE) {
         assert(tree_is(src) && "decl::variable::set_from_untyped must be given a var decl");
      }
      this->_node = src;
   }
         
   expr::declare variable::make_declare_expr(location_t source_location) {
      retrn expr::declare::create(*this, source_location);
   }
   
   expr::member_access variable::access_member(
      const char* field_name,
      location_t  access_location,
      location_t  field_name_location,
      location_t  arrow_operator_loc
   ) {
      return expr::member_access::create(
         *this,
         field_name,
         access_location,
         field_name_location,
         arrow_operator_loc
      );
   };
   
   /*static*/ variable variable::create(
      const char* identifier_name,
      const type& variable_type,
      location_t  source_location
   ) {
      variable out;
      out._node = build_decl(
         source_location,
         VAR_DECL,
         get_identifier(identifier_name),
         variable_type.as_untyped()
      );
      return out;
   }
}