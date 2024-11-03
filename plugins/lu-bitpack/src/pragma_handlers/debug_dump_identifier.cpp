#include "pragma_handlers/debug_dump_identifier.h"
#include <iostream>
#include <string_view>

#include <tree.h>
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier

#include <print-tree.h> // debug_tree

namespace pragma_handlers {
   extern void debug_dump_identifier(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack debug_dump_identifier";
      
      location_t loc;
      tree       data;
      auto token_type = pragma_lex(&data, &loc);
      if (token_type != CPP_NAME) {
         std::cerr << "error: " << this_pragma_name << ": not a valid identifier\n";
         return;
      }
      std::string_view name = IDENTIFIER_POINTER(data);
      
      auto id_node = get_identifier(name.data());
      auto decl    = lookup_name(id_node);
      if (decl == NULL_TREE) {
         
         //
         // Handle types that have no accompanying DECL node (e.g. RECORD_TYPE 
         // declared directly rather than with the typedef keyword).
         //
         auto node = identifier_global_tag(id_node);
         if (node && TYPE_P(node)) {
            debug_tree(node);
            return;
         }
         
         std::cerr << "error: " << this_pragma_name << ": identifier " << name << " not found\n";
         return;
      }
      debug_tree(decl);
   }
}