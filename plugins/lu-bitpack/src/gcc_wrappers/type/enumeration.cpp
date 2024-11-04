#include "gcc_wrappers/type/enumeration.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::type {
   WRAPPED_TREE_NODE_BOILERPLATE(enumeration)
   
   [[nodiscard]] std::vector<enumeration::member> enumeration::all_enum_members() const {
      std::vector<member> list;
      for_each_enum_member([&list](const member& item) {
         list.push_back(item);
      });
      return list;
   }
}