#pragma once
#include <gcc-plugin.h>
#include <tree.h>
#include <tree-iterator.h>

#include "gcc_wrappers/_wrapped_tree_node.h"
namespace gcc_wrappers {
   namespace expr {
      class base;
   }
}

namespace gcc_wrappers {
   class statement_list {
      protected:
         tree               _tree = NULL_TREE;
         tree_stmt_iterator _iterator;
         
      public:
         class iterator {
            friend statement_list;
            protected:
               tree_stmt_iterator _data;
            
            public:
               _wrapped_tree_node operator*();
               
               bool operator==(const iterator& other) const noexcept {
                  auto& a = this->_data;
                  auto& b = other._data;
                  if (a.container != b.container)
                     return false;
                  if (a.ptr != b.ptr)
                     return false;
                  return true;
               }
            
               iterator& operator--();
               iterator  operator--(int);
               iterator& operator++();
               iterator  operator++(int);
         };
         
      protected:
         statement_list(tree);
         
      public:
         statement_list();
         
         static statement_list view_of(tree);
         
         constexpr tree as_untyped() {
            return this->_tree;
         }
         
         iterator begin();
         iterator end();
         
         bool empty() const;
         
         void append(const expr::base&);
         void append(statement_list&&);
         void prepend(const expr::base&);
         void prepend(statement_list&&);
         
         void append_untyped(tree expr);
   };
}