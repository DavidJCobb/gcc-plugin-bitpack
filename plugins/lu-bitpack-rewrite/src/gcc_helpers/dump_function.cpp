#include "gcc_helpers/dump_function.h"
#include <iostream>
#include <print-tree.h> // debug_tree
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/decl/result.h"
#include "gcc_wrappers/list_node.h"
#include "gcc_wrappers/type/base.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}
 
namespace gcc_helpers {
   void dump_function(tree node) {
      if (node == NULL_TREE) {
         std::cerr << "Was asked to dump info for a function, but the decl doesn't exist.\n";
         return;
      }
      if (TREE_CODE(node) != FUNCTION_DECL) {
         std::cerr << "Was asked to dump info for a function, but the decl isn't a function.\n";
         std::cerr << " - Unqualified name: " << IDENTIFIER_POINTER(DECL_NAME(node)) << "\n";
         return;
      }
      
      auto decl = gw::decl::function::from_untyped(node);
      
      std::cerr << "Dumping information for function...\n";
      std::cerr << " - Unqualified name: " << decl.name() << "\n";
      {
         auto retn = decl.result_variable();
         if (retn.empty()) {
            std::cerr << " - Return type: void\n";
         } else {
            //
            // `retn` is basically an implicit variable, and assigning to it 
            // is how a function returns a value. Think of it like Result in 
            // some variants of Pascal/Delphi.
            //
            std::cerr << " - Return type: ";
            auto type = retn.value_type();
            if (type.empty()) {
               std::cerr << "<missing?>";
            } else {
               std::cerr << type.pretty_print();
            }
            std::cerr << '\n';
         }
      }
      std::cerr << " - Arguments:\n";
      
      auto args = DECL_ARGUMENTS(decl.as_untyped());
      if (args == NULL_TREE) {
         std::cerr << "    - <none>\n";
      }
      for(; args != NULL_TREE; args = TREE_CHAIN(args)) {
         if (TREE_CODE(args) != PARM_DECL) {
            std::cerr << "    - <malformed?>\n";
            return;
         }
         auto item = gw::decl::param::from_untyped(args);
         
         std::cerr << "    - ";
         std::cerr << item.name();
         std::cerr << ":\n";
         
         {  // Print type (simple only)
            std::cerr << "       - Type: ";
            std::cerr << item.value_type().pretty_print();
            std::cerr << '\n';
         }
         
         // print default value (faulty; always claims to have a default)
         if (auto expr = DECL_INITIAL(args); expr != NULL_TREE) {
            if (expr == error_mark_node) {
               std::cerr << "       - Has a default value (further info unavailable)\n";
            } else {
               std::cerr << "       - Has a default value\n";
            }
         }
      }
      
      std::cerr << " - Dumping function node...\n";
      debug_tree(decl.as_untyped());
      std::cerr << " - Dumping function body...\n";
      debug_tree(DECL_SAVED_TREE(decl.as_untyped()));
   }
}