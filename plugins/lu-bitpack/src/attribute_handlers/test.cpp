#include "attribute_handlers/bitpack_range.h"
#include <cinttypes> // PRIdMAX and friends
#include <cstdint>
#include "lu/strings/printf_string.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "debugprint.h"
#include <print-tree.h>
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree test(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      lu_debugprint("[lu_test_attribute] Argument summary:\n");
      
      size_t i = 0;
      for(auto node = args; node != NULL_TREE; ++i, node = TREE_CHAIN(node)) {
         auto text = lu::strings::printf_string(" - Arg %u:\n", (int)i);
         lu_debugprint(text);
         
         auto node_k = TREE_PURPOSE(node);
         auto node_v = TREE_VALUE(node);
         
         lu_debugprint("    - Key: ");
         if (node_k == NULL_TREE) {
            lu_debugprint(" <null>");
         } else {
            lu_debugprint(get_tree_code_name(TREE_CODE(node_k)));
         }
         lu_debugprint("\n");
         
         lu_debugprint("    - Value: ");
         if (node_v == NULL_TREE) {
            lu_debugprint(" <null>");
         } else {
            lu_debugprint(get_tree_code_name(TREE_CODE(node_v)));
         }
         lu_debugprint("\n");
      }
      
      lu_debugprint("[lu_test_attribute] Argument values in full:\n");
      i = 0;
      for(auto node = args; node != NULL_TREE; ++i, node = TREE_CHAIN(node)) {
         auto text = lu::strings::printf_string("ARG %u KEY AND VALUE NODES:\n", (int)i);
         lu_debugprint(text);
         
         auto node_k = TREE_PURPOSE(node);
         auto node_v = TREE_VALUE(node);
         
         if (node_k == NULL_TREE) {
            lu_debugprint("<null>\n");
         } else {
            debug_tree(node_k);
         }
         if (node_v == NULL_TREE) {
            lu_debugprint("<null>\n");
         } else {
            debug_tree(node_v);
         }
         lu_debugprint("\n");
      }
      
      *no_add_attrs = true;
      return NULL_TREE;
   }
}