#include "gcc_helpers/dump_function.h"
#include <iostream>
#include <cp/cp-tree.h>
#include <print-tree.h> // debug_tree
#include "gcc_helpers/for_each_in_list_tree.h"
#include "gcc_helpers/type_name.h"

// test
#include "gcc_helpers/compare_to_template_param.h"
 
namespace gcc_helpers {
   void dump_function(tree decl) {
      if (decl == NULL_TREE) {
         std::cerr << "Was asked to dump info for a function, but the decl doesn't exist.\n";
         return;
      }
      if (TREE_CODE(decl) != FUNCTION_DECL) {
         std::cerr << "Was asked to dump info for a function, but the decl isn't a function.\n";
         std::cerr << " - Unqualified name: " << IDENTIFIER_POINTER(DECL_NAME(decl)) << "\n";
         return;
      }
      std::cerr << "Dumping information for function...\n";
      std::cerr << " - Unqualified name: " << IDENTIFIER_POINTER(DECL_NAME(decl)) << "\n";
      /*if (DECL_FUNCTION_MEMBER_P(decl)) {
         //
         // Print what type this function is a member of, and information on 
         // what sort of member function it is.
         //
         const char* context_noun = nullptr;
         
         auto ctxt = DECL_CONTEXT(decl);
         switch (TREE_CODE(ctxt)) {
            case RECORD_TYPE:
               context_noun = "class or struct";
               break;
            case UNION_TYPE:
               context_noun = "union";
               break;
            default:
               break;
         }
         if (context_noun) {
            auto ctxt_name = type_name(ctxt);
            std::cerr << " - ";
            if (DECL_CONSTRUCTOR_P(decl)) {
               std::cerr << "Constructor of ";
            } else if (DECL_DESTRUCTOR_P(decl)) {
               std::cerr << "Destructor of ";
            } else if (DECL_CONV_FN_P(decl)) {
               std::cerr << "Conversion operator of ";
            } else {
               if (DECL_STATIC_FUNCTION_P(decl)) {
                  std::cerr << "Static member function of ";
               } else {
                  std::cerr << "Member function ";
                  if constexpr (false) { // this would handle C++
                     bool c = DECL_CONST_MEMFUNC_P(decl);
                     bool v = DECL_VOLATILE_MEMFUNC_P(decl);
                     if (c || v) {
                        std::cerr << '(';
                        //
                        bool needs_sep = false;
                        if (c) {
                           std::cerr << "const";
                           needs_sep = true;
                        }
                        if (v) {
                           if (needs_sep) {
                              needs_sep = false;
                              std::cerr << ' ';
                           }
                           std::cerr << "volatile";
                        }
                        //
                        std::cerr << ") ";
                     }
                  }
                  std::cerr << "of ";
               }
            }
            std::cerr << context_noun << ' ' << ctxt_name;
            std::cerr << '\n';
         }
      }*/
      if (auto retn = DECL_RESULT(decl); retn != NULL_TREE) {
         //
         // `retn` is basically an implicit variable, and assigning to it 
         // is how a function returns a value. Think of it like Result in 
         // some variants of Pascal/Delphi.
         //
         std::cerr << " - Return type: ";
         if (auto type = TREE_TYPE(retn); type != NULL_TREE) {
            std::cerr << type_name(type);
         } else {
            std::cerr << "<missing?>";
         }
         std::cerr << '\n';
      } else {
         std::cerr << " - Return type: void\n";
      }
      std::cerr << " - Arguments:\n";
      if(auto args = DECL_ARGUMENTS(decl); args != NULL_TREE) {
         for_each_in_list_tree(args, [](tree item) {
            if (TREE_CODE(item) != PARM_DECL) {
               std::cerr << "    - <malformed?>\n";
               return;
            }
            std::cerr << "    - ";
            std::cerr << IDENTIFIER_POINTER(DECL_NAME(item));
            std::cerr << ":\n";
            
            {  // Print type (simple only)
               std::cerr << "       - Type: ";
               
               size_t pointer_count = 0;
               auto   type = TREE_TYPE(item);
               while (type != NULL_TREE && TREE_CODE(type) == POINTER_TYPE) {
                  ++pointer_count;
                  type = TREE_TYPE(type);
               }
               if (TYPE_READONLY(type)) {
                  std::cerr << "const ";
               }
               std::cerr << type_name(type);
               for(size_t i = 0; i < pointer_count; ++i)
                  std::cerr << '*';
               
               std::cerr << '\n';
            }
            
            // print default value (faulty; always claims to have a default)
            if (auto expr = DECL_INITIAL(item); expr != NULL_TREE) {
               if (expr == error_mark_node) {
                  std::cerr << "       - Has a default value (further info unavailable)\n";
               } else {
                  std::cerr << "       - Has a default value\n";
               }
            }
         });
      } else {
         std::cerr << "    - <none>\n";
      }
      std::cerr << " - Dumping function body...\n";
      debug_tree(DECL_SAVED_TREE(decl));
      
      std::cerr << "\n\n";
      std::cerr << "Testing comparison (read) to template params...\n";
      
      using u8 = uint8_t;
      using arg_0   = const u8*;
      using arg_1   = int;
      using fn_type = void(*)(arg_0, arg_1);
      std::cerr << " - Whole function matches? ";
      std::cerr << gcc_helpers::function_decl_matches_template_param<fn_type>(decl); 
      std::cerr << '\n';
      {
         auto args = DECL_ARGUMENTS(decl);
         if (args != NULL_TREE) {
            std::cerr << " - Argument 0 matches? ";
            std::cerr << gcc_helpers::type_node_matches_template_param<arg_0>(TREE_TYPE(args));
            std::cerr << '\n';
            args = TREE_CHAIN(args);
            if (args != NULL_TREE) {
               std::cerr << " - Argument 1 matches? ";
               std::cerr << gcc_helpers::type_node_matches_template_param<arg_1>(TREE_TYPE(args));
               std::cerr << '\n';
            } else {
               std::cerr << " - Argument 1 not present.\n";
            }
         } else {
            std::cerr << " - Argument 0 not present.\n";
         }
      }
      
      
   }
}