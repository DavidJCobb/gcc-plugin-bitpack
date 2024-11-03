#include "pragma_handlers/debug_dump_function.h"

#include <tree.h>
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier

namespace pragma_handlers {
   extern void debug_dump_function(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack debug_dump_function";
      
      location_t loc;
      tree       data;
      auto token_type = pragma_lex(&data, &loc);
      if (token_type != CPP_NAME) {
         std::cerr << "error: " << this_pragma_name << ": not a valid identifier\n";
         return;
      }
      std::string_view name = IDENTIFIER_POINTER(data);
      
      auto decl = lookup_name(get_identifier(name.data()));
      if (decl == NULL_TREE) {
         std::cerr << "error: " << this_pragma_name << ": identifier " << name << " not found\n";
         return;
      }
      if (TREE_CODE(decl) != FUNCTION_DECL) {
         std::cerr << "error: " << this_pragma_name << ": identifier " << name << " is something other than a function\n";
         return;
      }
      gcc_helpers::dump_function(decl);
   }
}