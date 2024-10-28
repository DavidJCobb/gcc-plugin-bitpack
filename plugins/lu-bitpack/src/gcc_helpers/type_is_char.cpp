#include "gcc_helpers/type_is_char.h"
#include "gcc_helpers/for_each_transitive_typedef.h"
#include "gcc_helpers/type_min_max.h"
#include "gcc_helpers/type_name.h"

namespace gcc_helpers {
   extern bool type_is_char(tree type) {
      if (TREE_CODE(type) != INTEGER_TYPE)
         return false;
      //
      // Check numeric limits first.
      //
      if (TYPE_UNSIGNED(type))
         return false;
      {
         auto pair_here = gcc_helpers::type_min_max(type);
         auto pair_char = gcc_helpers::type_min_max(char_type_node);
         if (pair_here != pair_char)
            return false;
      }
      bool is_char = false;
      gcc_helpers::for_each_transitive_typedef(type, [&is_char](tree current) {
         auto name = gcc_helpers::type_name(current);
         if (name == "int8_t") {
            return false; // stop iterating
         }
         if (name == "char") {
            is_char = true;
            return false; // stop iterating
         }
         return true;
      });
      return is_char;
   }
}