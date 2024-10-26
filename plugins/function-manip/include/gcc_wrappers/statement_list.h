#pragma once
#include <gcc-plugin.h>
#include <tree.h>
#include <tree-iterator.h>

namespace gcc_wrappers {
   class statement_list {
      protected:
         tree               _tree = NULL_TREE;
         tree_stmt_iterator _iterator;
         
      public:
         statement_list();
         
         void append_untyped(tree expr);
   };
}