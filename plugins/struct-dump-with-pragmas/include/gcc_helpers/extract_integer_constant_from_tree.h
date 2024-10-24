#include <optional>
// GCC:
#include <gcc-plugin.h> 
#include <tree.h>

namespace gcc_helpers {
   template<typename T>
   extern std::optional<T> extract_integer_constant_from_tree(tree t) {
      if (!t || t == NULL_TREE)
         return {};
      if (TREE_CODE(t) != INTEGER_CST)
         return {};
      if (!TREE_CONSTANT(t))
         return {};
      return (T)TREE_INT_CST_LOW(t);
   }
}