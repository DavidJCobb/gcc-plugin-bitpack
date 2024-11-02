#pragma once
#include "gcc_wrappers/value.h"

namespace codegen {
   struct value_pair {
      gcc_wrappers::value read;
      gcc_wrappers::value save;
   };
}