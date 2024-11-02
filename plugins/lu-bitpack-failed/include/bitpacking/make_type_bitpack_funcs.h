#pragma once
#include <utility>
#include "gcc_wrappers/decl/function.h"

namespace codegen {
   extern std::pair<
      gcc_wrappers::decl::function, // read
      gcc_wrappers::decl::function  // write
   > make_type_bitpack_funcs(gcc_wrappers::type);
}