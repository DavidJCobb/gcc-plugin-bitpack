#pragma once
#include "codegen/expr_pair.h"
#include "codegen/func_pair.h"
#include "gcc_wrappers/expr/local_block.h"

namespace codegen {
   struct in_progress_func_pair : public func_pair {
      public:
         gcc_wrappers::expr::local_block read_root;
         gcc_wrappers::expr::local_block save_root;
         
      public:
         void append(expr_pair);
         void commit();
   };
}