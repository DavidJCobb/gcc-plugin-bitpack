#include "codegen/in_progress_func_pair.h"

#include <c-family/c-common.h> // lookup_name
// TEST
extern tree pushdecl (tree);
extern void c_bind (location_t loc, tree decl, bool is_global);
extern tree c_simulate_builtin_function_decl (tree decl);

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
      // It's something to do with the relationship between the top-level functions and 
      // the per-sector functions, but I just don't understand GCC's internals enough 
      // to figure out what's going on right now.
      
      this->read.as_modifiable().set_root_block(this->read_root);
      this->save.as_modifiable().set_root_block(this->save_root);
      
      {
         auto decl  = this->read;
         auto prior = lookup_name(DECL_NAME(decl.as_untyped()));
         if (prior == NULL_TREE) {
            // c_bind doesn't do what we need it to do: it doesn't ensure that the 
            // per-sector functions make it into the compiled object file.
            //c_bind(UNKNOWN_LOCATION, decl.as_untyped(), true);
            
            // Anything that calls pushdecl crashes as per the above.
            //pushdecl(decl.as_untyped());
            //c_simulate_builtin_function_decl(decl.as_untyped());
         }
      }
      {
         auto decl  = this->save;
         auto prior = lookup_name(DECL_NAME(decl.as_untyped()));
         if (prior == NULL_TREE) {
            // c_bind doesn't do what we need it to do: it doesn't ensure that the 
            // per-sector functions make it into the compiled object file.
            //c_bind(UNKNOWN_LOCATION, decl.as_untyped(), true);
            
            // Anything that calls pushdecl crashes as per the above.
            //pushdecl(decl.as_untyped());
            //c_simulate_builtin_function_decl(decl.as_untyped());
         }
      }
      
      //pushdecl(this->read.as_untyped());
      //pushdecl(this->save.as_untyped());
      
      //c_bind(UNKNOWN_LOCATION, this->read.as_untyped(), true);
      //c_bind(UNKNOWN_LOCATION, this->save.as_untyped(), true);
   }
}