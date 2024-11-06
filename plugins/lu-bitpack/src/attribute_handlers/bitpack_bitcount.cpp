#include "attribute_handlers/bitpack_bitcount.h"
#include "attribute_handlers/generic_type_or_decl.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/list_node.h"
#include <diagnostic.h>

namespace attribute_handlers {
   extern tree bitpack_bitcount(tree* node, tree name, tree raw_args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_type_or_decl(node, name, raw_args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      int v;
      {
         // The argument count should've been verified within GCC before we were called.
         auto arg = TREE_VALUE(raw_args);
         if (arg == NULL_TREE || TREE_CODE(arg) != INTEGER_CST) {
            error("%qE attribute argument must be an integer constant", name);
            goto failure;
         }
         auto opt = gcc_wrappers::expr::integer_constant::from_untyped(arg).try_value_signed();
         if (!opt.has_value()) {
            error("%qE attribute argument must be an integer constant", name);
            goto failure;
         }
         v = (int) *opt;
      }
      if (v <= 0) {
         error("%qE attribute argument cannot be zero or negative (seen: %<%d%>)", name, v);
         goto failure;
      }
      if (v > (int)sizeof(size_t) * 8) {
         warning(OPT_Wattributes, "%qE attribute specifies an unusually large bitcount; is this intentional? (seen: %<%d%>)", name, v);
      }
      
      *no_add_attrs = false;
      return NULL_TREE;
      
   failure:
      //*no_add_attrs = true; // actually, do add the argument, so codegen can fail later on
      return NULL_TREE;
   }
}