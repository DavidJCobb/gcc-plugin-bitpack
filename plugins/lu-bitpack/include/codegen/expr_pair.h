#pragma once
#include "gcc_wrappers/expr/base.h"

namespace codegen {
   struct expr_pair {
      gcc_wrappers::expr::base read;
      gcc_wrappers::expr::base save;
   };
}