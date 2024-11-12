#pragma once
#include <string_view>
#include <vector>
#include "gcc_wrappers/type/integral.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::type {
   class enumeration : public integral {
      public:
         static bool node_is(tree t) {
            return t != NULL_TREE && TREE_CODE(t) == ENUMERAL_TYPE;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(enumeration)
         
      public:
         struct member {
            std::string_view name;
            intmax_t         value = 0;
         };
         
      public:
         [[nodiscard]] std::vector<member> all_enum_members() const;
         
         bool is_opaque() const;
         bool is_scoped() const;
      
         template<typename Functor>
         void for_each_enum_member(Functor&& functor) const;
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"
#include "gcc_wrappers/type/enumeration.inl"