#include <iostream>
#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-common.h> // lookup_name; various predefined type nodes

#include "gcc_helpers/make_indexed_unbroken_for_loop.h"
#include "gcc_helpers/make_preincrement_expr.h"

void generate_read_code(tree function_decl) {
   tree state_type = NULL_TREE;
   {
      auto decl = lookup_name(get_identifier("lu_BitstreamState"));
      if (decl == NULL_TREE) {
         std::cerr << "ERROR: Failed to find bitstream-state type.";
         return;
      }
      if (TREE_CODE(decl) != TYPE_DECL) {
         std::cerr << "ERROR: Failed to find bitstream-state type (not a type decl).";
         return;
      }
      state_type = TREE_TYPE(decl);
   }
   tree state_decl = build_decl(
      UNKNOWN_LOCATION,
      VAR_DECL,
      get_identifier("__lu_bitstream_state"),
      state_type
   );
   TREE_USED(state_decl) = 1;
   DECL_ARTIFICIAL(state_decl) = 1;
   //TREE_CHAIN(state_decl) = some_next_variable_decl;
   
   tree buffer_decl = NULL_TREE;
   {
      static_assert(false, "TODO: get func arg");
   }
   
   auto func_root_expr  = DECL_SAVED_TREE(decl);
   auto func_root_vars  = TREE_OPERAND(func_root_expr, 0); // chain of VAR_DECL
   auto func_root_body  = TREE_OPERAND(func_root_expr, 1); // statement list
   auto func_root_block = TREE_OPERAND(func_root_expr, 2); // block
   
   // Concatenate our vars into the function vars.
   chainon(func_root_vars, state_decl);
   
   tree block = build_block( // make a BIND_EXPR
      state_decl,    // first variable declaration
      NULL_TREE,     // first sub-block (linked to next one via TREE_CHAIN)
      function_decl, // containing context (parent block or function decl)
      NULL_TREE      // chain (next-sibling block)
   );
   
   auto statements = alloc_stmt_list();
   append_to_statement_list(statements, build_stmt(UNKNOWN_LOCATION, DECL_EXPR, state_decl));
   {  // lu_BitstreamInitialize(&__lu_bitstream_state, buffer);
      auto call = build_call_expr(
      lookup_name(get_identifier("lu_BitstreamInitialize")), // function decl
         2 // argcount
         build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl, 0),
         buffer_decl
      );
      append_to_statement_list(statements, call);
   }
   
   {  // for (int i = 0; i < 9; ++i) { ... }
      tree loop_index; // VAR_DECL
      tree loop_body = alloc_statement_list();
      {
         loop_index = build_decl(
            UNKNOWN_LOCATION,
            VAR_DECL,
            get_identifier("__i"),
            uint8_type_node
         );
         DECL_ARTIFICIAL(loop_index) = 1;
      }
      {  // body statement: sStructA.a[i] = lu_BitstreamRead_u8(&state, 6);
         static_assert("TODO: dst_decl");
         static_assert("TODO: src_expr");
         auto assign = build_tree(
            MODIFY_EXPR,
            UNKNOWN_LOCATION,
            void_type_node,
            dst_decl,
            src_expr
         );
         append_to_statement_list(loop_body, assign);
      }
      auto for_loop = gcc_helpers::make_indexed_unbroken_for_loop(
         function_decl,
         loop_index,
         0,
         8,
         1,
         loop_body
      );
      append_to_statement_list(statements, for_loop);
   }
   
   static_assert(false, "TODO: FINISH ME");
   
}