#pragma once
#include <vector>
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/chain_node.h"
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
         chain_node all_members() const; // returns TREE_FIELDS(_node)
         
         // Loop over each FIELD_DECL in this object. This will only 
         // catch direct members and not, say, the contents of an 
         // anonymous object member.
         template<typename Functor>
         void for_each_field(Functor&& functor) const;
         
         // Loop over each FIELD_DECL in this object, and recursively 
         // loop over the FIELD_DECLs of any contained anonymous objects.
         template<typename Functor>
         void for_each_referenceable_field(Functor&& functor) const;
         
         template<typename Functor>
         void for_each_static_data_member(Functor&& functor) const;
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"
#include "gcc_wrappers/type/container.inl"