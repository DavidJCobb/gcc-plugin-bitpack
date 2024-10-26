#include <iostream>
#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-common.h> // lookup_name; various predefined type nodes

#include "gcc_helpers/compare_to_template_param.h"
#include "gcc_helpers/for_each_in_list_tree.h"
#include "gcc_helpers/make_indexed_unbroken_for_loop.h"
#include "gcc_helpers/make_member_access_expr.h"
#include "gcc_helpers/make_preincrement_expr.h"
#include "gcc_helpers/type_to_string.h"

void generate_read_code(tree function_decl) {
   struct {
      bitstream_initialize     = lookup_name(get_identifier("lu_BitstreamInitialize")),
      bitstream_read_bool      = lookup_name(get_identifier("lu_BitstreamRead_bool")),
      bitstream_read_u8        = lookup_name(get_identifier("lu_BitstreamRead_u8")),
      bitstream_read_u16       = lookup_name(get_identifier("lu_BitstreamRead_u16")),
      bitstream_read_u32       = lookup_name(get_identifier("lu_BitstreamRead_u32")),
      bitstream_read_string    = lookup_name(get_identifier("lu_BitstreamRead_string_optional_terminator")),
      bitstream_read_string_wt = lookup_name(get_identifier("lu_BitstreamRead_string")),
      bitstream_read_buffer    = lookup_name(get_identifier("lu_BitstreamRead_buffer")),
   } function_decls;
   #pragma region Validate function decls
   {
      auto _check_okay = []<typename FnPtr>(const char* name, tree decl) -> bool 
         if (decl == NULL_TREE) {
            std::cerr << "ERROR: Failed to find function: " << name << ".\n";
            return false;
         }
         if (TREE_CODE(decl) != FUNCTION_DECL) {
            std::cerr << "ERROR: Declaration is not a function: " << name << ".\n";
            return false;
         }
         if (!gcc_helpers::function_decl_matches_template_param<FnPtr>(decl)) {
            std::cerr << "ERROR: " << name << " does not have the expected signature.\n";
            return false;
         }
         return true;
      };
      
      if (!_check_okay<void(*)(lu_BitstreamState*, uint8_t*)>(
         "lu_BitstreamInitialize",
         function_decls.bitstream_initialize
      )) {
         return;
      }
      {
         constexpr const char* name = "lu_BitstreamRead_bool";
         auto decl = function_decls.bitstream_read_bool;
         
         if (decl == NULL_TREE) {
            std::cerr << "ERROR: Failed to find function: " << name << ".\n";
            return;
         }
         if (TREE_CODE(decl) != FUNCTION_DECL) {
            std::cerr << "ERROR: Declaration is not a function: " << name << ".\n";
            return;
         }
         if (!gcc_helpers::function_decl_matches_template_param<bool(*)(lu_BitstreamState*)>(decl)) {
            // handle pre-bool C
            if (!gcc_helpers::function_decl_matches_template_param<uint8_t(*)(lu_BitstreamState*)>(decl)) {
               std::cerr << "ERROR: " << name << " does not have the expected signature.\n";
               return;
            }
         }
      }
      if (!_check_okay<uint8_t(*)(lu_BitstreamState*, uint8_t)>(
         "lu_BitstreamRead_u8",
         function_decls.bitstream_read_u8
      )) {
         return;
      }
      if (!_check_okay<uint16_t(*)(lu_BitstreamState*, uint8_t)>(
         "lu_BitstreamRead_u16",
         function_decls.bitstream_read_u16
      )) {
         return;
      }
      if (!_check_okay<uint32_t(*)(lu_BitstreamState*, uint8_t)>(
         "lu_BitstreamRead_u32",
         function_decls.bitstream_read_u32
      )) {
         return;
      }
      if (!_check_okay<void(*)(lu_BitstreamState*, uint8_t*, uint16_t)>(
         "lu_BitstreamRead_string_optional_terminator",
         function_decls.bitstream_read_string
      )) {
         return;
      }
      if (!_check_okay<void(*)(lu_BitstreamState*, uint8_t*, uint16_t)>(
         "lu_BitstreamRead_string",
         function_decls.bitstream_read_string_wt
      )) {
         return;
      }
      if (!_check_okay<void(*)(lu_BitstreamState*, void*, uint16_t)>(
         "lu_BitstreamRead_buffer",
         function_decls.bitstream_read_buffer
      )) {
         return;
      }
   }
   #pragma endregion
   
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
   TREE_OPERAND(block, 1) = statements;
   
   {  // lu_BitstreamInitialize(&__lu_bitstream_state, buffer);
      auto call = build_call_expr(
         function_decls.bitstream_initialize,
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
      
         // sStructA.a[i]
         auto object_decl   = lookup_name(get_identifier("sStructA"));
         auto member_access = make_member_access_expr(
            object_decl,
            get_identifier("a")
         );
         auto array_access  = build_array_ref(
            UNKNOWN_LOCATION,
            member_access,
            loop_index
         );
         
         // lu_BitstreamRead_u8(&state, 6)
         tree read_call = build_call_expr(
            function_decls.bitstream_read_u8,
            2 // argcount
            build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl, 0),
            build_int_cst(uint8_type_node, 6) // arg: bitcount
         );
         
         auto assign = build_tree(
            MODIFY_EXPR,
            UNKNOWN_LOCATION,
            void_type_node,
            array_access, // dst
            read_call     // src
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
   
   // sStructA.b = lu_BitstreamRead_u8(&state, 5);
   {
      // sStructA.b
      auto object_decl   = lookup_name(get_identifier("sStructA"));
      auto member_access = make_member_access_expr(
         object_decl,
         get_identifier("b")
      );
      
      // lu_BitstreamRead_u8(&state, 5)
      tree read_call = build_call_expr(
         function_decls.bitstream_read_u8,
         2 // argcount
         build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl, 0),
         build_int_cst(uint8_type_node, 5) // arg: bitcount
      );
      
      auto assign = build_tree(
         MODIFY_EXPR,
         UNKNOWN_LOCATION,
         void_type_node,
         member_access, // dst
         read_call      // src
      );
      append_to_statement_list(statements, assign);
   }
   
   // lu_BitstreamRead_string(&state, sStructB.a, 5);
   {
      // sStructB.a
      auto object_decl   = lookup_name(get_identifier("sStructB"));
      auto member_access = make_member_access_expr(
         object_decl,
         get_identifier("a")
      );
      
      tree read_call = build_call_expr(
         function_decls.bitstream_read_string_wt,
         3 // argcount
         build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl, 0),
         member_access,
         build_int_cst(uint8_type_node, 5) // arg: max length
      );
      append_to_statement_list(statements, read_call);
   }
   
   if (!func_root_body) {
      TREE_OPERAND(func_root_expr) = statements;
   } else {
      append_to_statement_list(func_root_body, statements);
   }
}