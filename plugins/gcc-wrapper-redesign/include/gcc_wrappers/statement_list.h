#pragma once
#include "gcc_wrappers/node.h"
#include <tree-iterator.h>
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   namespace expr {
      class base;
   }
}

namespace gcc_wrappers {
   class statement_list : public node {
      public:
         static bool raw_node_is(tree t) {
            return TREE_CODE(t) == STATEMENT_LIST;
         }
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(statement_list)
         
         class iterator {
            friend statement_list;
            protected:
               tree_stmt_iterator _data;
            
            public:
               node operator*();
               
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
         void _require_not_moved_from() const;
         void _append(tree);
         void _prepend(tree);
         
      public:
         statement_list();
         
         iterator begin();
         iterator end();
         
         void append(const expr::base&);
         void append(statement_list&&);
         void prepend(const expr::base&);
         void prepend(statement_list&&);
         
         void append_untyped(node expr);
   };
   DECLARE_GCC_NODE_POINTER_WRAPPER(statement_list);
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"