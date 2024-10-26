#pragma once
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_helpers {
   //
   // Create a simple for-loop with no break or continue statements, 
   // which uses a numeric index for the loop condition.
   //
   extern tree make_indexed_unbroken_for_loop(
      tree     containing_function_decl,
      tree     index_decl,
      intmax_t begin,
      intmax_t last,
      intmax_t increment,
      tree     body // statement list
   );
}