#pragma once
#include "codegen/func_pair.h"

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