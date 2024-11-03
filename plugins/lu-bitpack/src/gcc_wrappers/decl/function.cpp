#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"
#include <cassert>
#include <stdexcept>

// function
namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(function)
   
   function::function(const char* name, const type& function_type) {
      this->_node = build_fn_decl(name, function_type.as_untyped());
   }
   function::function(const std::string& name, const type& function_type) {
      this->_node = build_fn_decl(name.c_str(), function_type.as_untyped());
   }
   
   type function::function_type() const {
      if (empty())
         return {};
      return type::from_untyped(TREE_TYPE(this->_node));
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
   
   bool function::has_body() const {
      assert(!empty());
      return DECL_SAVED_TREE(this->_node) != NULL_TREE;
   }
   
   function_with_modifiable_body function::as_modifiable() {
      assert(!empty());
      assert(!has_body());
      return function_with_modifiable_body(this->_node);
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
      DECL_SAVED_TREE(this->_node) = lb.as_untyped();
      
      auto root_block = make_node(BLOCK);
      BLOCK_SUPERCONTEXT(root_block) = this->_node;
      DECL_INITIAL(this->_node) = root_block;
      
      auto _traverse = [this](expr::local_block& lb, tree block) -> void {
         auto _impl = [this](expr::local_block& lb, tree block, auto& recurse) mutable -> void {
            tree prev_decl  = NULL_TREE;
            tree prev_child = NULL_TREE;
            
            auto statements = lb.statements();
            for(auto wrap : statements) {
               auto node = wrap.as_untyped();
               
               // Find and link all decls to the block, forming a 
               // linked list.
               if (TREE_CODE(node) == DECL_EXPR) {
                  node = DECL_EXPR_DECL(node);
                  if (TREE_CODE(node) == VAR_DECL) {
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
         };
         _impl(lb, block, _impl);
      };
      _traverse(lb, root_block);
   }
}