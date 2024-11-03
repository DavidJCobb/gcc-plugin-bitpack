#include "pragma_handlers/set_options.h"
#include "gcc_helpers/extract_pragma_kv_args.h"
#include "basic_global_state.h"

#include <tree.h>
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier

namespace pragma_handlers {
   extern void set_options(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack set_options";
      
      const auto kv = gcc_helpers::extract_pragma_kv_args(
         this_pragma_name,
         reader
      );
      try {
         basic_global_state::get().global_options.requested.load_pragma_args(kv);
      } catch (std::runtime_error& e) {
         std::cerr << "incorrect data in " << this_pragma_name << "\n";
         std::cerr << e.what();
      }
   }
}