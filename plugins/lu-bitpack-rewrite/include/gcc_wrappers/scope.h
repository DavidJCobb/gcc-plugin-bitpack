#pragma once
#include <string>
#include "gcc_wrappers/_wrapped_tree_node.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class base;
      class function;
   }
   namespace type {
      class base;
   }
}

namespace gcc_wrappers {
   class scope : public _wrapped_tree_node {
      public:
         static bool node_is(tree t);
         WRAPPED_TREE_NODE_BOILERPLATE(scope)
         
      public:
         scope() {}
         
         scope containing_scope();
         
         bool is_declaration() const;
         bool is_file_scope() const;
         bool is_namespace() const;
         bool is_type() const;
         
         decl::base as_declaration();
         type::base as_type();
         
         // Returns self, the nearest enclosing function, or empty.
         decl::function nearest_function();
         
         // Returns self, the nearest enclosing type, or empty.
         type::base nearest_type();
   };
}

#include "gcc_wrappers/_boilerplate.undef.h"