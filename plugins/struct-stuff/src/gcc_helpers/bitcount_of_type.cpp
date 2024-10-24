#include "gcc_helpers/bitcount_of_type.h"

namespace gcc_helpers {
   extern std::optional<size_t> bitcount_of_type(tree type) {
      tree size_tree = TYPE_SIZE(type);
      // NOTE: for size in bytes, use TYPE_SIZE_UNIT
      //
      // Fail on non-constant size:
      //
      if (TREE_CODE(size_tree) != INTEGER_CST)
         return {};
      if (TYPE_P(size_tree))
         return {};
      if (!TREE_CONSTANT(size_tree))
         return {};
      //
      return (size_t)TREE_INT_CST_NUNITS(size_tree);
      //return (size_t)TREE_INT_CST_LOW(size_tree);
   }
}