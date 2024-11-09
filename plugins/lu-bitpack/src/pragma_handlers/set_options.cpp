#include "pragma_handlers/set_options.h"
#include <iostream>
#include "gcc_helpers/extract_pragma_kv_args.h"
#include "gcc_wrappers/builtin_types.h"
#include "basic_global_state.h"

#include <tree.h>
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier
#include <diagnostic.h>

namespace pragma_handlers {
   extern void set_options(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack set_options";
      
      // ensure we can use `get_fast` on the built-in types:
      gcc_wrappers::builtin_types::get();
      
      auto& state = basic_global_state::get();
      if (state.global_options_seen) {
         error("%qs: global options have already been set in this translation unit and cannot be redefined", this_pragma_name);
      }
      
      const auto kv = gcc_helpers::extract_pragma_kv_args(
         this_pragma_name,
         reader
      );
      if (kv.empty())
         return;
      try {
         state.global_options.requested.consume_pragma_kv_set(kv);
         state.global_options.computed.resolve(state.global_options.requested);
      } catch (std::runtime_error& e) {
         error("%qs: incorrect data: %s", this_pragma_name, e.what());
      }
      state.global_options_seen = true;
   }
}