#pragma once
#include <gcc-plugin.h>
#include <c-family/c-pragma.h>

namespace pragma_handlers {
   extern void debug_dump_bp_data_options(cpp_reader*);
}