#include "pragma_handlers/heritable.h"
#include "bitpacking/heritable_options.h"

#include <tree.h>

namespace pragma_handlers {
   extern void heritable(cpp_reader* reader) {
      heritable_options_stockpile::get().handle_pragma(reader);
   }
}