#include "gcc_helpers/type_name.h"

namespace gcc_helpers {
   extern std::string type_name(tree type) {
      tree name_tree = TYPE_NAME(type);
      if (!name_tree) {
         return {}; // unnamed struct
      }
      if (TREE_CODE(name_tree) == IDENTIFIER_NODE)
         return IDENTIFIER_POINTER(name_tree);
      if (TREE_CODE(name_tree) == TYPE_DECL && DECL_NAME(name_tree))
         return IDENTIFIER_POINTER(DECL_NAME(name_tree));
      //
      // Name irretrievable?
      //
      return {};
   }
}