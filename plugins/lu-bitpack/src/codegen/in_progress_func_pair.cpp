#include "codegen/in_progress_func_pair.h"

namespace codegen {
   void in_progress_func_pair.append(expr_pair expr) {
      this->read_root.statements().append(expr.read);
      this->save_root.statements().append(expr.save);
   }
   void in_progress_func_pair.commit() {
      this->read.set_root_block(this->read_root);
      this->save.set_root_block(this->save_root);
   }
}