#include "gcc_wrappers/expr/local_block.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"

namespace gcc_wrappers::expr {
   WRAPPED_TREE_NODE_BOILERPLATE(local_block)
   
   local_block::local_block() {
      this->_node = build3(
         BIND_EXPR,
         void_type_node,    // return type
         NULL,              // vars (accessible via BIND_EXPR_VARS(expr))
         alloc_stmt_list(), // STATEMENT_LIST node
         NULL_TREE          // BLOCK node
      );
   }
   local_block::local_block(type::base desired) {
      this->_node = build3(
         BIND_EXPR,
         desired.as_untyped(), // return type
         NULL,                 // vars (accessible via BIND_EXPR_VARS(expr))
         alloc_stmt_list(),    // STATEMENT_LIST node
         NULL_TREE             // BLOCK node
      );
   }
   local_block::local_block(statement_list&& statements) {
      this->_node = build3(
         BIND_EXPR,
         void_type_node,          // return type
         NULL,                    // vars (accessible via BIND_EXPR_VARS(expr))
         statements.as_untyped(), // STATEMENT_LIST node
         NULL_TREE                // BLOCK node
      );
   }
   local_block::local_block(type::base desired, statement_list&& statements) {
      this->_node = build3(
         BIND_EXPR,
         desired.as_untyped(),    // return type
         NULL,                    // vars (accessible via BIND_EXPR_VARS(expr))
         statements.as_untyped(), // STATEMENT_LIST node
         NULL_TREE                // BLOCK node
      );
   }
   
   statement_list local_block::statements() {
      return statement_list::view_of(BIND_EXPR_BODY(this->_node));
   }
   
   void local_block::set_block_node(tree block) {
      if (block == NULL_TREE) {
         BIND_EXPR_BLOCK(this->_node) = NULL_TREE;
         return;
      }
      assert(TREE_CODE(block) == BLOCK);
      
      //
      // Per the `PROCESS_ARG` macro in `tree.cc`, used by `build3` and 
      // friends in `tree.h`; and per `build3` itself:
      //
      BIND_EXPR_BLOCK(this->_node) = block;
      if (TREE_SIDE_EFFECTS(block))
         TREE_SIDE_EFFECTS(this->_node) = 1;
      
      // Guessed:
      BIND_EXPR_VARS(this->_node) = BLOCK_VARS(block);
   }
}