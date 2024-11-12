#include "pragma_handlers/set_options.h"
#include "basic_global_state.h"

namespace pragma_handlers {
   extern void enable(cpp_reader* reader) {
      basic_global_state::get().enabled = true;
   }
}