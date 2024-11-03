#include "codegen/in_progress_func_pair.h"

namespace codegen {
   void in_progress_func_pair::append(expr_pair expr) {
      this->read_root.statements().append(expr.read);
      this->save_root.statements().append(expr.save);
   }
   void in_progress_func_pair::commit() {
      
      // These lines are what allow the top-level functions to emit to the object file.
      // Theoretically, uncommenting them would also allow the per-sector functions to 
      // emit to the object file. In practice, however, we just crash instead.
      //this->read.set_is_defined_elsewhere(false);
      //this->save.set_is_defined_elsewhere(false);
      
      this->read.as_modifiable().set_root_block(this->read_root);
      this->save.as_modifiable().set_root_block(this->save_root);
   }
}