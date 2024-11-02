#include "gcc_helpers/type_size_in_bytes.h"

static bool _is_constant(tree t) {
   if (TREE_CODE(t) != INTEGER_CST)
      return false;
   //if (TYPE_P(t))
   //   return false;
   if (!TREE_CONSTANT(t))
      return false;
   return true;
}

namespace gcc_helpers {
   extern std::optional<size_t> type_size_in_bits(tree type) {
      tree size_tree = TYPE_SIZE(type);
      if (!_is_constant(size_tree))
         return{};
      return (size_t)TREE_INT_CST_LOW(size_tree);
   }
   extern std::optional<size_t> type_size_in_bytes(tree type) {
      tree size_tree = TYPE_SIZE_UNIT(type);
      if (!_is_constant(size_tree))
         return{};
      return (size_t)TREE_INT_CST_LOW(size_tree);
   }
}