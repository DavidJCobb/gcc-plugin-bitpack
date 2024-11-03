#include "codegen/in_progress_func_pair.h"

namespace codegen {
   void in_progress_func_pair::append(expr_pair expr) {
      this->read_root.statements().append(expr.read);
      this->save_root.statements().append(expr.save);
   }
   void in_progress_func_pair::commit() {
      //
      // First, some needed changes in order for our functions to be emitted to 
      // the object files.
      //
      this->read.set_is_defined_elsewhere(false);
      this->save.set_is_defined_elsewhere(false);
      
      this->read.as_modifiable().set_root_block(this->read_root);
      this->save.as_modifiable().set_root_block(this->save_root);
      
      //
      // Finally, some cleanup. To the best of my knowledge, these actions are 
      // also required for the functions to be emitted. We do them conditionally 
      // so that if we're generating a definition for a function that was already 
      // forward-declared in source, we don't conflict with the declaration.
      //
      this->read.introduce_to_current_scope();
      this->save.introduce_to_current_scope();
   }
}