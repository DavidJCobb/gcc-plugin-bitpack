#include "gcc_wrappers/type/helpers/lookup_by_name.h"
#include <c-family/c-common.h> // identifier_global_tag, lookup_name
#include <stringpool.h> // get_identifier

namespace gcc_wrappers::type {
   extern base lookup_by_name(const char* name) {
      auto id_node = get_identifier(name);
      auto node    = lookup_name(id_node);
      //
      // The `lookup_name` function returns a `*_DECL` node with the given 
      // identifier. However, TYPE_DECLs only exist in C when the `typedef` 
      // keyword is used. To find normal struct definitions, we need to use 
      // another approach.
      //
      if (node == NULL_TREE) {
         //
         // That "other approach" is to look for a "tag."
         //
         node = identifier_global_tag(id_node);
         if (node == NULL_TREE)
            return {};
      }
      if (TREE_CODE(node) == TYPE_DECL) {
         node = TREE_TYPE(node);
      } else if (!TYPE_P(node)) {
         return {};
      }
      return base::from_untyped(node);
   }
   extern base lookup_by_name(const std::string& s) {
      return lookup_by_name(s.c_str());
   }
}