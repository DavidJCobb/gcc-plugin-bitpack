#include "gcc_helpers/array_extent.h"

namespace gcc_helpers {
   extern std::optional<size_t> array_extent(tree type) {
      if (TREE_CODE(type) != ARRAY_TYPE)
         return {};
      auto domain = TYPE_DOMAIN(type);
      if (domain == NULL_TREE)
         return {};
      auto max = TYPE_MAX_VALUE(domain);
      //
      // Fail on non-constant:
      //
      if (!max)
         return {};
      if (TREE_CODE(max) != INTEGER_CST)
         return {};
      if (TYPE_P(max))
         return {};
      if (!TREE_CONSTANT(max))
         return {};
      //
      return (size_t)TREE_INT_CST_LOW(max) + 1;
   }
}