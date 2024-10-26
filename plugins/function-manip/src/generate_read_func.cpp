#include "generate_read_func.h"
#include <cassert>
// GCC plug-ins:
#include <gcc-plugin.h>
#include <tree.h>
#include <cp/cp-tree.h> // build_array_ref; etc.
#include <tree-iterator.h> // alloc_stmt_list; append_to_statement_list; etc.
#include <c-family/c-common.h> // lookup_name; c_build_qualified_type; various predefined type nodes
#include <c-family/c-pragma.h>
#include <stringpool.h> // get_identifier

#include "gcc_helpers/compare_to_template_param.h"
#include "gcc_helpers/make_indexed_unbroken_for_loop.h"
#include "gcc_helpers/make_member_access_expr.h"

#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/expr/declare.h"
#include "gcc_wrappers/expr/member_access.h"
#include "gcc_wrappers/statement_list.h"
#include "gcc_wrappers/type.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

#include "bp_func_decl_set.h"

/*

   Goal is for the following to supply the body of the given function:
   
      #pragma lu_bitpack generate_read( \
         function_identifier = generated_read, \
         data = ( sStructA, sStructB, sStructC ) \
      )
   
   Should generate as:
   
      void generated_read(const u8* src, int sector_id);

*/

void generate_read_func(const generate_request& request) {
   assert(!request.generated_function_identifier.empty());
   
   bp_func_decl_set needed_functions;
   if (!needed_functions.check_all()) {
      return;
   }
   
   tree func_decl = NULL_TREE;
   {
      const char* name = request.generated_function_identifier.c_str();
      
      func_decl = lookup_name(get_identifier(name));
      if (func_decl == NULL_TREE) {
         warning(OPT_Wpragmas, "to-be-generated bitpack-read function with name %<%s%> has not been declared in this scope; generating an implicit declaration", name);
         
         gw buffer_type;
         gw.set_from_untyped(uint8_type_node);
         gw = gw.add_pointer().add_const_qualifier();
         
         auto func_type = build_function_type_list(
            void_type_node,
            // start of args
            gw.as_untyped(),
            integer_type_node, // sector_id
            // end of args
            NULL_TREE
         );
         func_decl = build_fn_decl(name, func_type);
      } else {
         if (TREE_CODE(func_decl) != FUNCTION_DECL) {
            error("cannot generate bitpack-read function with name %<%s%>, as something else already exists with that name", name);
            return;
         }
         if (DECL_SAVED_TREE(func_decl) != NULL_TREE) {
            error("bitpack-read function with name %<%s%> is already defined i.e. already has a body; cannot generate a body", name);
            return;
         }
         if (!gcc_helpers::function_decl_matches_template_param<void(*)(const uint8_t*, int)>(func_decl)) {
            error("bitpack-read function with name %<%s%> was already declared but with a signature other than expected; cannot generate a body", name);
            return;
         }
      }
   }
   
   //
   // Codegen below based in part on:
   //  - https://github.com/gcc-mirror/gcc/blob/ecf80e7daf7f27defe1ca724e265f723d10e7681/gcc/function-tests.cc#L220
   //
   
   // Declare return variable.
   tree retn_decl = build_decl(UNKNOWN_LOCATION, RESULT_DECL, NULL_TREE, void_type_node);
   DECL_ARTIFICIAL(retn_decl) = 1;
   DECL_IGNORED_P(retn_decl) = 1;
   DECL_RESULT(func_decl) = retn_decl;
   
   tree               statements = alloc_stmt_list();
   tree_stmt_iterator stmt_iter  = tsi_start(statements);
   
   tree root_block     = make_node(BLOCK);
   tree root_bind_expr = build3(BIND_EXPR, void_type_node, NULL, statements, root_block);
   
   tree state_type = NULL_TREE;
   {
      auto node = identifier_global_tag(get_identifier("lu_BitstreamState"));
      if (node == NULL_TREE) {
         error("failed to find bitstream state type");
         return;
      }
      switch (TREE_CODE(node)) {
         case TYPE_DECL:
            state_type = TREE_TYPE(node);
            break;
         case RECORD_TYPE:
            state_type = node;
            break;
         default:
            error("failed to find bitstream state type (found declaration was not a type declaration)");
            return;
      }
   }
   
   auto state_decl = gw::variable::create("__lu_bitstream_state", state_type);
   state_decl.mark_artificial();
   state_decl.mark_used();
   {
      auto declare = state_decl.make_declare_expr();
      tsi_link_after(&stmt_iter, declare.as_untyped(), TSI_CONTINUE_LINKING);
   }
   
   {  // for (int i = 0; i < 9; ++i) { ... }
      auto loop_index = gw::variable::create("__i", uint8_type_node);
      loop_index.mark_artificial();
      loop_index.mark_used();
   
      tree loop_body = alloc_stmt_list();
      {  // body statement: sStructA.a[i] = lu_BitstreamRead_u8(&state, 6);
      
         // sStructA.a[i]
         gw::decl::variable object_decl;
         object_decl.set_from_untyped(lookup_name(get_identifier("sStructA")));
         
         auto member_access = object_decl.access_member("a");
         auto array_access  = build_array_ref(
            UNKNOWN_LOCATION,
            member_access.as_untyped(),
            loop_index.as_untyped()
         );
         
         // lu_BitstreamRead_u8(&state, 6)
         tree read_call = build_call_expr(
            needed_functions.bitstream_read_u8.decl,
            2, // argcount
            build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl.as_untyped(), 0),
            build_int_cst(uint8_type_node, 6) // arg: bitcount
         );
         
         auto assign = build2(
            MODIFY_EXPR,
            void_type_node,
            array_access, // dst
            read_call     // src
         );
         append_to_statement_list(loop_body, &assign);
      }
      auto for_loop = gcc_helpers::make_indexed_unbroken_for_loop(
         func_decl,
         loop_index.as_untyped(),
         0,
         8,
         1,
         loop_body
      );
      //append_to_statement_list(statements, for_loop);
      tsi_link_after(&stmt_iter, for_loop, TSI_CONTINUE_LINKING);
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
         build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl.as_untyped(), 0),
         build_int_cst(uint8_type_node, 5) // arg: bitcount
      );
      
      auto assign = build2(
         MODIFY_EXPR,
         void_type_node,
         member_access, // dst
         read_call      // src
      );
      //append_to_statement_list(statements, assign);
      tsi_link_after(&stmt_iter, assign, TSI_CONTINUE_LINKING);
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
         build_unary_op(UNKNOWN_LOCATION, ADDR_EXPR, state_decl.as_untyped(), 0),
         member_access,
         build_int_cst(uint8_type_node, 5) // arg: max length
      );
      //append_to_statement_list(statements, read_call);
      tsi_link_after(&stmt_iter, read_call, TSI_CONTINUE_LINKING);
   }
   
   /*//
   tree modify_retval = build2(MODIFY_EXPR,
      integer_type_node,
      retn_decl,
      build_int_cst(integer_type_node, 42)
   );
   tree retn_stmt = build1(RETURN_EXPR, integer_type_node, modify_retval);
   //append_to_statement_list(statements, retn_stmt);
   tsi_link_after(&stmt_iter, read_call, TSI_CONTINUE_LINKING);
   //*/

   DECL_INITIAL(func_decl) = root_block;
   BLOCK_SUPERCONTEXT(root_block) = func_decl;
   DECL_SAVED_TREE(func_decl) = root_bind_expr;

   // Ensure that locals appear in the debuginfo.
   BLOCK_VARS(root_block) = BIND_EXPR_VARS(root_bind_expr);
}