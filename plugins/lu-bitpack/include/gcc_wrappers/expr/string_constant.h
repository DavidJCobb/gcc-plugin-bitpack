#pragma once
#include <string>
#include <string_view>
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      class string_constant : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == STRING_CST;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(string_constant)
            
         public:
            // CAN_HAVE_LOCATION_P is false for all nodes of this type, so calls 
            // to this member function would replace `this->_node` with a pointer 
            // of a different type.
            void set_source_location(location_t, bool wrap_if_necessary = false);
         
         public:
            string_constant() {}
            string_constant(std::string_view);
            
            std::string value() const;
            std::string_view value_view() const;
      };
      static_assert(sizeof(string_constant) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"