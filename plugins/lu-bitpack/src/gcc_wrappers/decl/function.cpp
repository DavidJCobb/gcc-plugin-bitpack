#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/decl/param.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"
#include <cassert>
#include <stdexcept>
#include "lu/strings/printf_string.h"
#include <function.h>
#include <stringpool.h> // get_identifier
#include <output.h> // assemble_external
#include <toplev.h>
#include <c-family/c-common.h> // lookup_name, pushdecl, etc.
#include <cgraph.h>

// This is in `c/c-tree.h`, which isn't included among the plug-in headers.
extern tree pushdecl(tree);

// function
namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(function)
   
   void function::_make_decl_arguments_from_type() {
      gcc_assert(!empty());
      
      size_t i = 0;
      
      auto args = this->function_type().arguments();
      tree prev = NULL_TREE;
      for(auto pair : args) {
         if (pair.second == NULL_TREE) {
            break;
         }
         if (TREE_CODE(pair.second) == VOID_TYPE) {
            //
            // Sentinel indicating that this is not a varargs function.
            //
            break;
         }
         
         // Generate a name for the arguments, to avoid crashes on a 
         // null IDENTIFIER_NODE when dumping function info elsewhere.
         auto arg_name = lu::strings::printf_string("__arg_%u", i++);
         
         auto p = param::from_untyped(build_decl(
            UNKNOWN_LOCATION,
            PARM_DECL,
            get_identifier(arg_name.c_str()),
            pair.second
         ));
         DECL_ARG_TYPE(p.as_untyped()) = pair.second;
         DECL_CONTEXT(p.as_untyped()) = this->_node;
         if (prev == NULL_TREE) {
            DECL_ARGUMENTS(this->_node) = p.as_untyped();
         } else {
            TREE_CHAIN(prev) = p.as_untyped();
         }
         prev = p.as_untyped();
      }
   }
   
   function::function(const char* name, const type::function& function_type) {
      //
      // Create the node with GCC's defaults (defined elsewhere, extern, 
      // artificial, nothrow).
      //
      this->_node = build_fn_decl(name, function_type.as_untyped());
      //
      // NOTE: That doesn't automatically build DECL_ARGUMENTS. We need 
      // to do so manually.
      //
      this->_make_decl_arguments_from_type();
   }
   function::function(const std::string& name, const type::function& function_type) : function(name.c_str(), function_type) {
   }
   
   type::function function::function_type() const {
      if (empty())
         return {};
      return type::function::from_untyped(TREE_TYPE(this->_node));
   }
   param function::nth_parameter(size_t n) const {
      assert(!empty());
      
      size_t i    = 0;
      tree   decl = DECL_ARGUMENTS(this->_node);
      for(; decl != NULL_TREE; decl = TREE_CHAIN(decl)) {
         if (i == n)
            break;
         ++i;
      }
      if (i == n)
         return param::from_untyped(decl);
      throw std::out_of_range("out-of-bounds function_decl parameter access");
   }
   result function::result_variable() const {
      return result::from_untyped(DECL_RESULT(this->_node));
   }

   bool function::is_always_emitted() const {
      if (empty())
         return false;
      return DECL_PRESERVE_P(this->_node);
   }
   void function::make_always_emitted() {
      set_is_always_emitted(true);
   }
   void function::set_is_always_emitted(bool) {
      assert(!empty());
      DECL_PRESERVE_P(this->_node) = 1;
   }
   
   bool function::is_defined_elsewhere() const {
      if (empty())
         return false;
      return DECL_EXTERNAL(this->_node);
   }
   void function::set_is_defined_elsewhere(bool v) {
      assert(!empty());
      DECL_EXTERNAL(this->_node) = v ? 1 : 0;
   }
            
   bool function::is_externally_accessible() const {
      if (empty())
         return false;
      return TREE_PUBLIC(this->_node);
   }
   void function::make_externally_accessible() {
      set_is_externally_accessible(true);
   }
   void function::set_is_externally_accessible(bool v) {
      assert(!empty());
      TREE_PUBLIC(this->_node) = v ? 1 : 0;
   }

   bool function::is_noreturn() const {
      if (empty())
         return false;
      return TREE_THIS_VOLATILE(this->_node);
   }
   void function::make_noreturn() {
      set_is_noreturn(true);
   }
   void function::set_is_noreturn(bool v) {
      assert(!empty());
      TREE_THIS_VOLATILE(this->_node) = v ? 1 : 0;
   }
   
   bool function::is_nothrow() const {
      if (empty())
         return false;
      return TREE_NOTHROW(this->_node);
   }
   void function::make_nothrow() {
      set_is_nothrow(true);
   }
   void function::set_is_nothrow(bool v) {
      assert(!empty());
      TREE_NOTHROW(this->_node) = v ? 1 : 0;
   }
   
   bool function::has_body() const {
      assert(!empty());
      return DECL_SAVED_TREE(this->_node) != NULL_TREE;
   }
   
   function_with_modifiable_body function::as_modifiable() {
      assert(!empty());
      assert(!has_body());
      {
         auto args = DECL_ARGUMENTS(this->_node);
         if (args == NULL_TREE) {
            this->_make_decl_arguments_from_type();
         } else {
            //
            // Update contexts. In the case of us taking a forward-declared 
            // function and generating a definition for it, the PARM_DECLs 
            // may exist but lack the appropriate DECL_CONTEXT.
            //
            for(; args != NULL_TREE; args = TREE_CHAIN(args)) {
               DECL_CONTEXT(args) = this->_node;
            }
         }
      }
      return function_with_modifiable_body(this->_node);
   }
   
   void function::introduce_to_current_scope() {
      auto prior = lookup_name(DECL_NAME(this->as_untyped()));
      if (prior == NULL_TREE) {
         pushdecl(this->as_untyped());
      }
   }
}

// Avoid warnings on later includes:
#include "gcc_wrappers/_boilerplate.undef.h"

#include "gcc_wrappers/decl/result.h"
#include "gcc_wrappers/expr/local_block.h"
#include <tree-iterator.h>

// function_with_modifiable_body
namespace gcc_wrappers::decl {
   function_with_modifiable_body::function_with_modifiable_body(tree n)
      :
      function(_modifiable_subconstruct_tag{})
   {
      this->_node = n;
      assert(node_is(n));
   }
   
   void function_with_modifiable_body::set_result_decl(result decl) {
      DECL_RESULT(this->_node) = decl.as_untyped();
      DECL_CONTEXT(decl.as_untyped()) = this->_node;
   }
   
   void function_with_modifiable_body::set_root_block(expr::local_block& lb) {
      //
      // The contents of a function are stored in two ways:
      //
      //  - `DECL_SAVED_TREE(func_decl)` is a tree of expressions representing the 
      //    code inside of the function: variable declarations, statements, and so 
      //    on. In this tree, blocks are represented as `BIND_EXPR` nodes.
      //
      //  - `DECL_INITIAL(func_decl)` is a tree of scopes. Each scope is represented
      //    as a `BLOCK` node.
      //
      //    Properties include:
      //
      //       BLOCK_VARS(block)
      //          The first *_DECL node scoped to this block. The TREE_CHAIN of this 
      //          decl is the next decl scoped to this block; and onward.
      //       
      //       BLOCK_SUBBLOCKS(block)
      //          The block's first child.
      //       
      //       BLOCK_CHAIN(block)
      //          The block's next sibling.
      //       
      //       BLOCK_SUPERCONTEXT(block)
      //          The parent BLOCK or FUNCTION_DECL node.
      //
      gcc_assert(!lb.empty());
      gcc_assert(BIND_EXPR_BLOCK(lb.as_untyped()) == NULL_TREE && "This local block seems to already be in use elsewhere.");
      
      DECL_SAVED_TREE(this->_node) = lb.as_untyped();
      TREE_STATIC(this->_node) = 1;
      
      auto root_block = make_node(BLOCK);
      BLOCK_SUPERCONTEXT(root_block) = this->_node;
      DECL_INITIAL(this->_node) = root_block;
      
      DECL_CONTEXT(DECL_RESULT(this->_node)) = this->_node;
      
      auto _traverse = [this](expr::local_block& lb, tree block) -> void {
         auto _impl = [this](expr::local_block& lb, tree block, auto& recurse) mutable -> void {
            tree prev_decl  = NULL_TREE;
            tree prev_child = NULL_TREE;
            
            bool side_effects = TREE_SIDE_EFFECTS(lb.as_untyped());
            auto statements   = lb.statements();
            for(auto wrap : statements) {
               auto node = wrap.as_untyped();
               
               if (TREE_SIDE_EFFECTS(node)) {
                  side_effects = true;
               }
               
               // Find and link all decls to the block, forming a 
               // linked list.
               if (TREE_CODE(node) == DECL_EXPR) {
                  node = DECL_EXPR_DECL(node);
                  if (TREE_CODE(node) == VAR_DECL) {
                     DECL_CONTEXT(node) = this->_node; // also required
                     
                     assert(TREE_CHAIN(node) == NULL_TREE);
                     if (prev_decl) {
                        TREE_CHAIN(prev_decl) = node;
                     } else {
                        BLOCK_VARS(block) = node;
                        BIND_EXPR_VARS(lb.as_untyped()) = node;
                     }
                     prev_decl = node;
                  }
                  continue;
               }
               
               if (TREE_CODE(node) == LABEL_EXPR) {
                  auto label_decl = LABEL_EXPR_LABEL(node);
                  DECL_CONTEXT(label_decl) = this->_node;
                  continue;
               }
               
               if (TREE_CODE(node) == BIND_EXPR) {
                  auto child = make_node(BLOCK);
                  BLOCK_SUPERCONTEXT(child) = block;
                  
                  auto casted = expr::local_block::from_untyped(node);
                  recurse(casted, child, recurse);
                  
                  // Link child blocks as a linked list.
                  if (prev_child) {
                     BLOCK_CHAIN(prev_child) = child;
                  } else {
                     BLOCK_SUBBLOCKS(block) = child;
                  }
                  prev_child = child;
                  continue;
               }
            }
            
            // Must be done after we've recursed, so it can properly propagate 
            // the BLOCK node's vars to the BIND_EXPR node. Keeping the two 
            // lists matched is necessary to ensure that locals appear in the 
            // debuginfo. [1]
            //
            // [1] https://github.com/gcc-mirror/gcc/blob/ecf80e7daf7f27defe1ca724e265f723d10e7681/gcc/function-tests.cc#L220
            //
            lb.set_block_node(block);
            TREE_SIDE_EFFECTS(lb.as_untyped()) = side_effects ? 1 : 0;
         };
         _impl(lb, block, _impl);
      };
      _traverse(lb, root_block);
      
      //
      // Update contexts. In the case of us taking a forward-declared 
      // function and generating a definition for it, the PARM_DECLs 
      // may exist but lack the appropriate DECL_CONTEXT.
      //
      for(auto args = DECL_ARGUMENTS(this->_node); args != NULL_TREE; args = TREE_CHAIN(args)) {
         DECL_CONTEXT(args) = this->_node;
      }
      
      //
      // Actions below are needed in order to emit the function to the object file, 
      // so the linker can see it and it's included in the program.
      //
      // NOTE: You must also mark the function as non-extern, manually.
      //
      
      if (!DECL_STRUCT_FUNCTION(this->_node)) {
         //
         // We need to allocate a "struct function" (i.e. a struct that describes 
         // some stuff about the function) in order for the function to be truly 
         // complete. (Unclear on what purpose this serves exactly, but I do know 
         // that the GCC functions to debug-print a whole FUNCTION_DECL won't even 
         // attempt to print the PARM_DECLs unless a struct function is present.) 
         //
         // Allocating a struct function triggers "layout" of the FUNCTION_DECL 
         // and its associated DECLs, including the RESULT_DECL. As such, if there 
         // is no RESULT_DECL, then GCC crashes.
         //
         gcc_assert(DECL_RESULT(this->_node) != NULL_TREE);
         allocate_struct_function(this->_node, false); // function.h
      }
      
      // per `finish_function` in `c/c-decl.cc`:
      if (!decl_function_context(this->_node)) {
         // re: our issue with crashing when trying to get the per-sector functions 
         // into the object file: this branch doesn't make a difference in that 
         // regard.
         if constexpr (false) {
            cgraph_node::add_new_function(this->_node, false);
         } else {
            cgraph_node::finalize_function(this->_node, false); // cgraph.h
         }
      } else {
         //
         // We're a nested function.
         //
         cgraph_node::get_create(this->_node);
      }
   }
}