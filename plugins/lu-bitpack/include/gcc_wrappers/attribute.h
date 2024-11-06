#pragma once
#include <string_view>
#include "lu/strings/zview.h"
#include "gcc_wrappers/_wrapped_tree_node.h"
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/chain_node.h"

namespace gcc_wrappers {
   
   // Wraps a key/value pair in an attribute list. The key is the 
   // attribute name identifier, and the value is the arguments 
   // passed, if any.
   class attribute : public _wrapped_tree_node {
      public:
         static attribute from_untyped(tree t) {
            attribute out;
            out._node = t;
            return out;
         }
      
      public:
         class arguments_wrapper {
            //
            // Attribute arguments can be either an expression list, 
            // or an identifier (that does not name a type) followed 
            // by an expression list.
            //
            // This wrapper exposes the identifier argument separately 
            // from the others. Things like `begin`, `end`, and `size` 
            // only apply to the expression arguments.
            //
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
                     expr::base operator*();
                  
                     iterator& operator++();
                     iterator  operator++(int) const;
                     
                     bool operator==(const iterator&) const = default;
               };
            
            public:
               bool empty() const noexcept;
            
               bool has_leading_identifier() const;
               const_tree leading_identifier_node() const;
               std::string_view leading_identifier() const;
               
               size_t size() const noexcept;
               expr::base operator[](size_t); // returns empty if out of range
               
               expr::base front();
               expr::base back();
         
               iterator begin();
               iterator end();
               iterator at(size_t n); // may throw std::out_of_range
         };
      
      public:
         std::string_view name() const;
         std::string_view namespace_name() const; // i.e. `"foo"` for `[[foo::bar]]`
         
         bool is_cpp11() const; // i.e. `[[foo::bar]]`
         
         bool compare_values(const attribute) const;
         
         arguments_wrapper arguments();
   };
   
   class attribute_list {
      protected:
         tree _node = NULL_TREE;
         
      public:
         inline tree as_untyped() const {
            return this->_node;
         }
         
      public:
         class iterator {
            friend attribute_list;
            protected:
               iterator(tree);
               
               tree _node = NULL_TREE;
               
            public:
               attribute operator*();
            
               iterator& operator++();
               iterator  operator++(int) const;
               
               bool operator==(const iterator&) const = default;
         };
         
      public:
         static attribute_list from_untyped(tree t);
      
         bool contains(const attribute) const;
         
         inline bool empty() const noexcept {
            return this->_node == NULL_TREE;
         }
         
         iterator begin();
         iterator end();
         iterator at(size_t n); // may throw std::out_of_range
      
         attribute first_attribute_with_prefix(lu::strings::zview);
      
         attribute get_attribute(lu::strings::zview);
      
         bool has_attribute(lu::strings::zview) const;
         
         void remove_attribute(lu::strings::zview);
         
         bool operator==(const attribute_list) const noexcept;
   };
}