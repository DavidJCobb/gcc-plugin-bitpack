#include "pragma_handlers/set_options.h"
#include <iostream>
#include "gcc_helpers/extract_pragma_kv_args.h"
#include "basic_global_state.h"

#include <tree.h>
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier
#include <diagnostic.h>

namespace pragma_handlers {
   extern void set_options(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack set_options";
      
      const auto kv = gcc_helpers::extract_pragma_kv_args(
         this_pragma_name,
         reader
      );
      try {
         basic_global_state::get().global_options.requested.consume_pragma_kv_set(kv);
      } catch (std::runtime_error& e) {
         error("incorrect data in %<%s%>: %s", this_pragma_name, e.what());
      }
   }
}