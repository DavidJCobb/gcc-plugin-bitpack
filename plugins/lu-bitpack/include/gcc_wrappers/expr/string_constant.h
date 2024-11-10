#pragma once
#include <string>
#include <string_view>
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/type/base.h"
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
            string_constant(const std::string&);
            string_constant(const char*);
            string_constant(const char*, size_t size_including_null);
            
            std::string value() const;
            std::string_view value_view() const;
            
            size_t length() const; // number of characters; excludes null terminator
            size_t size_of() const; // number of characters; includes null terminator
            
            // Each STRING_CST can only have a single type, but can be referenced 
            // by multiple ADDR_EXPRs (i.e. multiple string literals can share 
            // the same data).
            bool referenced_by_string_literal() const;
            
            // Creates a new string literal. If this STRING_CST is already used 
            // by a literal of a different type, then we'll have to clone the 
            // STRING_CST.
            //
            // If the input type is empty, we default to `char` (and we assert 
            // in that case that the built-in type nodes are actually ready).
            ::gcc_wrappers::value to_string_literal(type::base character_type);
      };
      static_assert(sizeof(string_constant) == sizeof(value)); // no new fields
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"