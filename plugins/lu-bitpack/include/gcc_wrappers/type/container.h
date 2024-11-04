#pragma once
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers::type {
   class container : public base {
      public:
         static bool node_is(tree t) {
            if (t == NULL_TREE)
               return false;
            switch (TREE_CODE(t)) {
               case RECORD_TYPE:
               case UNION_TYPE:
                  return true;
               default:
                  break;
            }
            return false;
         }
         WRAPPED_TREE_NODE_BOILERPLATE(container)
         
      public:
         list_node all_members() const; // returns TREE_FIELDS(_node)
      
         template<typename Functor>
         void for_each_field(Functor&& functor) const;
         
         template<typename Functor>
         void for_each_static_data_member(Functor&& functor) const;
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"
#include "gcc_wrappers/type/container.inl"