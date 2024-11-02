#pragma once
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_helpers {
   // Create a COMPONENT_EXPR given a struct-or-union instance DECL (e.g. VAR_DECL)
   // and the IDENTIFIER_NODE of a field (e.g. `get_identifier("member_name")`).
   tree make_member_access_expr(
      tree object_decl,
      tree field_identifier // IDENTIFIER_NODE
   );
   
   tree make_member_access_expr(
      location_t member_access_loc,
      tree       object_decl,
      tree       field_identifier, // IDENTIFIER_NODE
      location_t field_identifier_loc,
      location_t arrow_operator_loc
   );
}