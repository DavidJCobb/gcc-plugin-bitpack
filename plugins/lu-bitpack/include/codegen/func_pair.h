#pragma once
#include "gcc_wrappers/decl/function.h"

namespace codegen {
   struct func_pair {
      gcc_wrappers::decl::function read;
      gcc_wrappers::decl::function save;
   };
}