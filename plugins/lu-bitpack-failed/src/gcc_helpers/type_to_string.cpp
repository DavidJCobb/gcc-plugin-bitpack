#include "gcc_helpers/type_to_string.h"
#include "gcc_helpers/type_name.h"

namespace gcc_helpers {
   extern std::string type_to_string(tree type) {
      std::string out;
      if (TYPE_READONLY(type))
         out = "const ";
      
      size_t pointer_count = 0;
      while (type != NULL_TREE && TREE_CODE(type) == POINTER_TYPE) {
         ++pointer_count;
         type = TREE_TYPE(type);
      }
      if (type == NULL_TREE)
         return {};
      
      out += type_name(type);
      if (pointer_count > 0) {
         out.reserve(out.size() + pointer_count);
         for(size_t i = 0; i < pointer_count; ++i)
            out += '*';
      }
      
      return out;
   }
}