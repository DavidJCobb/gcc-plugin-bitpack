#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/param.h"
#include "gcc_wrappers/decl/result.h"
#include "gcc_wrappers/type.h"
#include "gcc_wrappers/_boilerplate.define.h"

namespace gcc_wrappers {
   namespace decl {
      class result;
   }
   namespace expr {
      class local_block;
   }
}

namespace gcc_wrappers {
   namespace decl {
      class function_with_modifiable_body;
      
      class function : public base {
         public:
            static bool node_is(tree t) {
               return TREE_CODE(t) == FUNCTION_DECL;
            }
            WRAPPED_TREE_NODE_BOILERPLATE(function)
            
         protected:
            enum class _modifiable_subconstruct_tag {};
            function(_modifiable_subconstruct_tag) {}
            
            void _make_decl_arguments_from_type();
            
         public:
            // Creates an empty wrapper.
            function() {}
            function(std::nullptr_t) {}
         
            // Creates a new FUNCTION_DECL. To wrap an existing one, 
            // use the static `from_untyped` member function.
            function(
               const char* identifier_name,
               const type& function_type
            );
            function(
               const std::string& identifier_name,
               const type& function_type
            );
            
            type function_type() const;
            param nth_parameter(size_t) const;
            result result_variable() const;
            
            // DECL_PRESERVE_P
            bool is_always_emitted() const;
            void make_always_emitted();
            void set_is_always_emitted(bool);
            
            // DECL_EXTERNAL
            //
            // NOTE: a C99 "extern inline" function will be marked as 
            // being defined elsewhere yet still have a definition in 
            // the current translation unit.
            bool is_defined_elsewhere() const;
            void set_is_defined_elsewhere(bool);
            
            // Indicates that this decl's name is to be accessible from 
            // outside of the current translation unit.
            bool is_externally_accessible() const; // TREE_PUBLIC
            void make_externally_accessible();
            void set_is_externally_accessible(bool);
            
            bool is_noreturn() const; // TREE_THIS_VOLATILE
            void make_noreturn();
            void set_is_noreturn(bool);
            
            bool is_nothrow() const;
            void make_nothrow();
            void set_is_nothrow(bool);
            
            bool has_body() const;
            
            // assert(!has_body());
            function_with_modifiable_body as_modifiable();
      };
      
      class function_with_modifiable_body : public function {
         friend class function;
         protected:
            function_with_modifiable_body(tree);
         
         public:
            void set_result_decl(result);
            void set_root_block(expr::local_block&);
      };
   }
}

#include "gcc_wrappers/_boilerplate.undef.h"