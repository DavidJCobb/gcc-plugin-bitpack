#pragma once
#include "gcc_wrappers/node.h"
#include "gcc_wrappers/node_pointer_template.h"
#include "gcc_wrappers/_node_ref_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class base;
      class function;
      using function_ptr = node_pointer_template<function>;
   }
   namespace type {
      class base;
      using base_ptr = node_pointer_template<base>;
   }
}

namespace gcc_wrappers {
   class scope;
   DECLARE_GCC_NODE_POINTER_WRAPPER(scope);
   
   class scope : public node {
      public:
         static bool raw_node_is(tree t);
         GCC_NODE_REFERENCE_WRAPPER_BOILERPLATE(scope)
         
      public:
         scope_ptr containing_scope();
         
         bool is_declaration() const;
         bool is_file_scope() const;
         bool is_namespace() const;
         bool is_type() const;
         
         decl::base as_declaration();
         type::base as_type();
         
         // Returns self, the nearest enclosing function, or nullptr.
         decl::function_ptr nearest_function();
         
         // Returns self, the nearest enclosing type, or nullptr.
         type::base_ptr nearest_type();
   };
}

#include "gcc_wrappers/_node_ref_boilerplate.undef.h"