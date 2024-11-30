#pragma once
#include <string_view>
#include "lu/strings/zview.h"
#include "gcc_wrappers/node.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   // Wraps a key/value pair in an attribute list. The key is the 
   // attribute name identifier, and the value is the arguments 
   // passed, if any.
   class attribute : public node {
      public:
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(attribute)
         
         class arguments_wrapper {
            friend attribute;
            protected:
               arguments_wrapper(tree t);
            
               tree _node = NULL_TREE; // TREE_VALUE(attr_node)
            
            public:
               class iterator {
                  friend arguments_wrapper;
                  protected:
                     iterator(tree);
                     
                     tree _node = NULL_TREE;
                     
                  public:
                     node operator*();
                  
                     iterator& operator++();
                     iterator  operator++(int) const;
                     
                     bool operator==(const iterator&) const = default;
               };
            
            public:
               bool empty() const noexcept;
               
               size_t size() const noexcept;
               node operator[](size_t); // may throw std::out_of_range
               
               node front();
               node back();
               const node front() const;
               const node back() const;
         
               iterator begin();
               iterator end();
               iterator at(size_t n); // may throw std::out_of_range
         };
      
      public:
         attribute(lu::strings::zview name, list args);
      
         std::string_view name() const;
         std::string_view namespace_name() const; // i.e. `"foo"` for `[[foo::bar]]`
         
         bool is_cpp11() const; // i.e. `[[foo::bar]]`
         
         bool compare_values(const attribute) const;
         
         arguments_wrapper arguments();
         const arguments_wrapper arguments() const;
   };
   DECLARE_GCC_NODE_POINTER_WRAPPER(attribute);
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"