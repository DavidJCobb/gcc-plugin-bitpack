#include "gcc_helpers/type_min_max.h"

namespace gcc_helpers {
   extern std::pair<
      std::optional<intmax_t>,
      std::optional<uintmax_t>
   > type_min_max(tree type) {
      std::pair<
         std::optional<intmax_t>,
         std::optional<uintmax_t>
      > out;
      
      auto _bound = []<typename T>(tree t) -> std::optional<T> {
         if (!t)
            return {};
         if (TREE_CODE(t) != INTEGER_CST)
            return {};
         if (TYPE_P(t))
            return {};
         if (!TREE_CONSTANT(t))
            return {};
         return (T)TREE_INT_CST_LOW(t);
      };
      out.first  = _bound.operator()<intmax_t>(TYPE_MIN_VALUE(type));
      out.second = _bound.operator()<intmax_t>(TYPE_MAX_VALUE(type));
      
      return out;
   }
}