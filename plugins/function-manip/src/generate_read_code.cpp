#include <iostream>
#include <gcc-plugin.h>
#include <tree.h>

// build_array_ref
#include <cp/cp-tree.h>

#include <tree-iterator.h> // alloc_stmt_list, append_to_statement_list
#include <c-family/c-common.h> // lookup_name; various predefined type nodes
#include <stringpool.h> // get_identifier

#include "gcc_helpers/compare_to_template_param.h"
#include "gcc_helpers/for_each_in_list_tree.h"
#include "gcc_helpers/make_indexed_unbroken_for_loop.h"
#include "gcc_helpers/make_member_access_expr.h"
#include "gcc_helpers/make_preincrement_expr.h"
#include "gcc_helpers/type_to_string.h"

namespace {
   template<typename FnPtr>
   struct expected_func_decl {
      expected_func_decl(const char* name) {
         this->decl = lookup_name(get_identifier(name));
      }
      
      const char* name = nullptr;
      tree        decl = NULL_TREE;
      
      bool check() const {
         if (this->decl == NULL_TREE) {
            std::cerr << "ERROR: Failed to find function: " << this->name << ".\n";
            return false;
         }
         if (TREE_CODE(this->decl) != FUNCTION_DECL) {
            std::cerr << "ERROR: Declaration is not a function: " << this->name << ".\n";
            return false;
         }
         if (!gcc_helpers::function_decl_matches_template_param<FnPtr>(this->decl)) {
            std::cerr << "ERROR: " << this->name << " does not have the expected signature.\n";
            return false;
         }
         return true;
      }
   };
}

void generate_read_code(tree function_decl) {
   struct {
      expected_func_decl<void(*)(struct lu_BitstreamState*, uint8_t*)>
      bitstream_initialize{"lu_BitstreamInitialize"};
      
      expected_func_decl<uint8_t(*)(struct lu_BitstreamState*)>
      bitstream_read_bool{"lu_BitstreamRead_bool"};
      
      expected_func_decl<uint8_t(*)(struct lu_BitstreamState*, uint8_t)>
      bitstream_read_u8{"lu_BitstreamRead_u8"};
      
      expected_func_decl<uint16_t(*)(struct lu_BitstreamState*, uint8_t)>
      bitstream_read_u16{"lu_BitstreamRead_u16"};
      
      expected_func_decl<uint32_t(*)(struct lu_BitstreamState*, uint8_t)>
      bitstream_read_u32{"lu_BitstreamRead_u32"};
      
      expected_func_decl<void(*)(struct lu_BitstreamState*, uint8_t*, uint16_t)>
      bitstream_read_string{"lu_BitstreamRead_string_optional_terminator"};
      
      expected_func_decl<void(*)(struct lu_BitstreamState*, uint8_t*, uint16_t)>
      bitstream_read_string_wt{"lu_BitstreamRead_string"};
      
      expected_func_decl<void(*)(struct lu_BitstreamState*, void*, uint16_t)>
      bitstream_read_buffer{"lu_BitstreamRead_buffer"};
   } needed_functions;
   
   if (!needed_functions.bitstream_initialize.check())
      return;
   if (!needed_functions.bitstream_read_bool.check())
      return;
   if (!needed_functions.bitstream_read_u8.check())
      return;
   if (!needed_functions.bitstream_read_u16.check())
      return;
   if (!needed_functions.bitstream_read_u32.check())
      return;
   if (!needed_functions.bitstream_read_string.check())
      return;
   if (!needed_functions.bitstream_read_string_wt.check())
      return;
   if (!needed_functions.bitstream_read_buffer.check())
      return;
   
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
   
   auto func_root_expr  = DECL_SAVED_TREE(function_decl);
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
   auto declare_state_stmt = build_stmt(UNKNOWN_LOCATION, DECL_EXPR, state_decl);
   append_to_statement_list(statements, &declare_state_stmt);
   TREE_OPERAND(block, 1) = statements;
   
   {  // lu_BitstreamInitialize(&__lu_bitstream_state, buffer);
      auto call = build_call_expr(
         needed_functions.bitstream_initialize.decl,
         2, // argcount
         build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl, 0),
         buffer_decl
      );
      append_to_statement_list(statements, &call);
   }
   
   {  // for (int i = 0; i < 9; ++i) { ... }
      tree loop_index; // VAR_DECL
      tree loop_body = alloc_stmt_list();
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
         auto member_access = gcc_helpers::make_member_access_expr(
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
            needed_functions.bitstream_read_u8.decl,
            2, // argcount
            build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl, 0),
            build_int_cst(uint8_type_node, 6) // arg: bitcount
         );
         
         auto assign = build2(
            MODIFY_EXPR,
            void_type_node, // ? may need to be the type we're assigning ?
            array_access,   // dst
            read_call       // src
         );
         append_to_statement_list(loop_body, &assign);
      }
      auto for_loop = gcc_helpers::make_indexed_unbroken_for_loop(
         function_decl,
         loop_index,
         0,
         8,
         1,
         loop_body
      );
      append_to_statement_list(statements, &for_loop);
   }
   
   // sStructA.b = lu_BitstreamRead_u8(&state, 5);
   {
      // sStructA.b
      auto object_decl   = lookup_name(get_identifier("sStructA"));
      auto member_access = gcc_helpers::make_member_access_expr(
         object_decl,
         get_identifier("b")
      );
      
      // lu_BitstreamRead_u8(&state, 5)
      tree read_call = build_call_expr(
         needed_functions.bitstream_read_u8.decl,
         2, // argcount
         build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl, 0),
         build_int_cst(uint8_type_node, 5) // arg: bitcount
      );
      
      auto assign = build2(
         MODIFY_EXPR,
         void_type_node, // ? may need to be the type we're assigning ?
         member_access,  // dst
         read_call       // src
      );
      append_to_statement_list(statements, &assign);
   }
   
   // lu_BitstreamRead_string(&state, sStructB.a, 5);
   {
      // sStructB.a
      auto object_decl   = lookup_name(get_identifier("sStructB"));
      auto member_access = gcc_helpers::make_member_access_expr(
         object_decl,
         get_identifier("a")
      );
      
      tree read_call = build_call_expr(
         needed_functions.bitstream_read_string_wt.decl,
         3, // argcount
         build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl, 0),
         member_access,
         build_int_cst(uint8_type_node, 5) // arg: max length
      );
      append_to_statement_list(statements, &read_call);
   }
   
   if (!func_root_body) {
      TREE_OPERAND(func_root_expr, 1) = statements;
   } else {
      append_to_statement_list(func_root_body, &statements);
   }
}