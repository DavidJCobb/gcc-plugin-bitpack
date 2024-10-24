#include <string>
// GCC:
#include <gcc-plugin.h> 
#include <tree.h>

namespace gcc_helpers {
   inline std::string extract_string_constant_from_tree(tree t) {
      if (!t || t == NULL_TREE)
         return {};
      if (TREE_CODE(t) != STRING_CST)
         return {};
      if (!TREE_CONSTANT(t))
         return {};
      
      auto size_of = TREE_STRING_LENGTH(t); // includes null
      if (size_of == 0)
         return {};
      return std::string(TREE_STRING_POINTER(t), size_of - 1);
   }
}