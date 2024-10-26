#include "gcc_helpers/make_member_access_expr.h"
#include <plugin-version.h>
#include <c-tree.h> // build_component_ref

namespace gcc_helpers {
   tree make_member_access_expr(
      tree object_decl,
      tree field_identifier // IDENTIFIER_NODE
   ) {
      return make_member_access_expr(UNKNOWN_LOCATION, object_decl, field_identifier, UNKNOWN_LOCATION, UNKNOWN_LOCATION);
   }
   
   tree make_member_access_expr(
      location_t member_access_loc,
      tree       object_decl,
      tree       field_identifier, // IDENTIFIER_NODE
      location_t field_identifier_loc,
      location_t arrow_operator_loc
   ) {
      #if GCCPLUGIN_VERSION_MAJOR >= 9 && GCCPLUGIN_VERSION_MINOR >= 5
         #if GCCPLUGIN_VERSION_MAJOR >= 13 && GCCPLUGIN_VERSION_MINOR >= 3
            // https://github.com/gcc-mirror/gcc/commit/bb49b6e4f55891d0d8b596845118f40df6ae72a5
            return build_component_ref(
               member_access_loc,
               object_decl,
               field_identifier,
               field_identifier_loc,
               arrow_operator_loc,
               true // default; todo: use false for offsetof, etc.?
            );
         #else
            // GCC PR91134
            // https://github.com/gcc-mirror/gcc/commit/7a3ee77a2e33b8b8ad31aea27996ebe92a5c8d83
            return build_component_ref(
               member_access_loc,
               object_decl,
               field_identifier,
               field_identifier_loc,
               arrow_operator_loc
            );
         #endif
      #else
         return build_component_ref(
            member_access_loc,
            object_decl,
            field_identifier,
            field_identifier_loc
         );
      #endif
   }
}