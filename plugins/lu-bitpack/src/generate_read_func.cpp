#include "generate_read_func.h"
#include <cassert>
// GCC plug-ins:
#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-common.h> // lookup_name; c_build_qualified_type; various predefined type nodes
#include <c-family/c-pragma.h>
#include <stringpool.h> // get_identifier

#include "gcc_helpers/compare_to_template_param.h"

#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/decl/result.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/expr/declare.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/local_block.h"
#include "gcc_wrappers/expr/return_result.h"
#include "gcc_wrappers/flow/simple_for_loop.h"
#include "gcc_wrappers/statement_list.h"
#include "gcc_wrappers/type.h"
#include "gcc_wrappers/value.h"
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
   
   struct {
      gw::type t_int     = gw::type::from_untyped(integer_type_node);
      gw::type t_void    = gw::type::from_untyped(void_type_node);
      gw::type t_uint8_t = gw::type::from_untyped(uint8_type_node);
   } types;
   
   gw::decl::function func_decl;
   {
      const char* name = request.generated_function_identifier.c_str();
      
      auto decl = lookup_name(get_identifier(name));
      if (decl != NULL_TREE) {
         if (TREE_CODE(decl) != FUNCTION_DECL) {
            error("cannot generate bitpack-read function with name %<%s%>, as something else already exists with that name", name);
            return;
         }
         if (DECL_SAVED_TREE(decl) != NULL_TREE) {
            error("bitpack-read function with name %<%s%> is already defined i.e. already has a body; cannot generate a body", name);
            return;
         }
         if (!gcc_helpers::function_decl_matches_template_param<void(*)(const uint8_t*, int)>(decl)) {
            error("bitpack-read function with name %<%s%> was already declared but with a signature other than expected; cannot generate a body", name);
            return;
         }
         func_decl.set_from_untyped(decl);
      } else {
         warning(OPT_Wpragmas, "to-be-generated bitpack-read function with name %<%s%> has not been declared in this scope; generating an implicit declaration", name);
         
         auto func_type = gw::type::make_function_type(
            types.t_void,
            // args:
            types.t_uint8_t.add_pointer().add_const(), // src
            types.t_int                                // sector_id
         );
         func_decl = gw::decl::function(name, func_type);
      }
   }
   auto func_dst = func_decl.as_modifiable();
   
   //
   // Codegen below based in part on:
   //  - https://github.com/gcc-mirror/gcc/blob/ecf80e7daf7f27defe1ca724e265f723d10e7681/gcc/function-tests.cc#L220
   //
   
   // Declare return variable.
   gw::decl::result retn_decl(gw::type::from_untyped(void_type_node));
   func_dst.set_result_decl(retn_decl);
   
   gw::expr::local_block root_block;
   auto statements = root_block.statements();
   
   gw::type state_type;
   {
      auto node = identifier_global_tag(get_identifier("lu_BitstreamState"));
      if (node == NULL_TREE) {
         error("failed to find bitstream state type");
         return;
      }
      switch (TREE_CODE(node)) {
         case TYPE_DECL:
            state_type.set_from_untyped(TREE_TYPE(node));
            break;
         case RECORD_TYPE:
            state_type.set_from_untyped(node);
            break;
         default:
            error("failed to find bitstream state type (found declaration was not a struct type declaration)");
            return;
      }
   }
   
   auto state_decl = gw::decl::variable("__lu_bitstream_state", state_type);
   state_decl.make_artificial();
   state_decl.make_used();
   //
   statements.append(state_decl.make_declare_expr());
   
   {  // lu_BitstreamInitialize(&state, src);
      auto src_arg = func_decl.nth_parameter(0).as_value();
      {
         auto pointer_type = src_arg.value_type();
         auto value_type   = pointer_type.remove_pointer();
         if (value_type.is_const()) {
            src_arg = src_arg.conversion_sans_bytecode(value_type.remove_const().add_pointer());
         }
      }
      statements.append(
         gw::expr::call(
            gw::decl::function::from_untyped(needed_functions.bitstream_initialize.decl),
            // args:
            state_decl.as_value().address_of(), // &state
            src_arg // src
         )
      );
   }
   
   {  // for (int i = 0; i <= 8; ++i) { ... }
      gw::flow::simple_for_loop loop(types.t_int);
      gw::statement_list        loop_body;
      loop.counter_bounds.start     = 0;
      loop.counter_bounds.last      = 8;
      loop.counter_bounds.increment = 1;
      {
         // sStructA
         auto object_decl = gw::decl::variable::from_untyped(lookup_name(get_identifier("sStructA")));
         
         loop_body.append(gw::expr::assign(
            // sStruct.a[i]
            object_decl.as_value().access_member("a").access_array_element(loop.counter.as_value()),
            // =
            // lu_BitstreamRead_u8(&state, 6)
            gw::expr::call(
               gw::decl::function::from_untyped(needed_functions.bitstream_read_u8.decl),
               // args:
               state_decl.as_value().address_of(),            // &state
               gw::expr::integer_constant(types.t_uint8_t, 6) // bitcount
            )
         ));
      }
      loop.bake(std::move(loop_body));
      statements.append(loop.enclosing);
   }
   
   // sStructA.b = lu_BitstreamRead_u8(&state, 5);
   {
      // sStructA
      auto object_decl = gw::decl::variable::from_untyped(lookup_name(get_identifier("sStructA")));
      
      statements.append(
         gw::expr::assign(
            // sStructA[b]
            object_decl.as_value().access_member("b"),
            // =
            // lu_BitstreamRead_u8(&state, 5)
            gw::expr::call(
               gw::decl::function::from_untyped(needed_functions.bitstream_read_u8.decl),
               // args:
               state_decl.as_value().address_of(),            // &state
               gw::expr::integer_constant(types.t_uint8_t, 5) // bitcount
            )
         )
      );
   }
   
   // lu_BitstreamRead_string(&state, sStructB.a, 5);
   {
      // sStructB
      auto object_decl = gw::decl::variable::from_untyped(lookup_name(get_identifier("sStructB")));
      
      statements.append(
         gw::expr::call(
            gw::decl::function::from_untyped(needed_functions.bitstream_read_string_wt.decl),
            // args:
            state_decl.as_value().address_of(),            // &state
            object_decl.as_value().access_member("a").convert_array_to_pointer(), // sStructB.a
            gw::expr::integer_constant(types.t_uint8_t, 5) // max length
         )
      );
   }
   
   /*// for reference: how to `return 42`
   statements.append(
      gw::expr::return_result(
         gw::expr::assign(
            retn_decl,
            gw::expr::integer_constant(integer_type_node, 42)
         )
      )
   );
   //*/
   
   func_dst.set_root_block(root_block);
}