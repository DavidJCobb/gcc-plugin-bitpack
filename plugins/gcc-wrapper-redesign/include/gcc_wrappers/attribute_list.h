#pragma once
#include "lu/strings/zview.h"
#include "gcc_wrappers/node.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   class attribute;
}

namespace gcc_wrappers {
   class attribute_list {
      protected:
         node_ptr _head;
      
      public:
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(attribute_list)
         
         class iterator {
            friend attribute_list;
            protected:
               iterator(node_ptr);
               
               node_ptr _node = NULL_TREE;
               
            public:
               attribute operator*();
            
               iterator& operator++();
               iterator  operator++(int) const;
               
               bool operator==(const iterator&) const = default;
         };
         
      public:
         // Equality comparison.
         bool operator==(const attribute_list) const noexcept;
         
         bool contains(const attribute) const;
         bool empty() const;
         
         iterator begin();
         iterator end();
         iterator at(size_t n); // may throw std::out_of_range
      
         // Returns the first attribute whose name starts with the given substring.
         attribute_ptr first_attribute_with_prefix(lu::strings::zview);
         const attribute_ptr first_attribute_with_prefix(lu::strings::zview) const;
      
         // Returns the first attribute with the given name.
         attribute_ptr get_attribute(lu::strings::zview);
         const attribute_ptr get_attribute(lu::strings::zview) const;
      
         bool has_attribute(lu::strings::zview) const;
         
         // assert(TREE_CODE(id_node) == IDENTIFIER_NODE)
         bool has_attribute(tree id_node) const;
         
         void remove_attribute(lu::strings::zview);
   };
   DECLARE_GCC_NODE_POINTER_WRAPPER(attribute_list);
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"