#pragma once
#include <string_view>
#include "gcc_wrappers/_wrapped_tree_node.h"
#include "gcc_wrappers/type.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class base : public _wrapped_tree_node {
         public:
            static bool node_is(tree t) {
               return DECL_P(t);
            }
            WRAPPED_TREE_NODE_BOILERPLATE(base)
            
         public:
            std::string_view name() const;
            
            std::string_view source_file() const;
            int source_line() const;
            
            bool is_artificial() const;
            void make_artificial();
            void set_is_artificial(bool);
            
            bool is_sym_debugger_ignored() const; // DECL_IGNORED_P
            void make_sym_debugger_ignored();
            void set_is_sym_debugger_ignored(bool);
            
            bool is_used() const;
            void make_used();
            void set_is_used(bool);
            
            // TODO: what types can this be?
            _wrapped_tree_node context() const;
      };
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"