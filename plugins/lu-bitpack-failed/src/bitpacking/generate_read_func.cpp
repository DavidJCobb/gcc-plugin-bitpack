#pragma once
#include "parse_generate_request.h"

#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-common.h> // lookup_name; various predefined type nodes
#include <stringpool.h>        // get_identifier
#include <diagnostic.h>

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

extern void generate_read_func(
   const global_bitpack_options& options,
   const generate_request&       request
) {
   if (request.generated_function_identifier.empty()) {
      error("cannot generate bitpack-read function; no name specified");
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
   
   // Declare return variable.
   gw::decl::result retn_decl(gw::type::from_untyped(void_type_node));
   func_dst.set_result_decl(retn_decl);
   
   gw::expr::local_block root_block;
   auto statements = root_block.statements();
   
   gw::type state_type;
   {
      auto  func_name = request.generated_function_identifier.c_str();
      auto& type_name = options.types.bitstream_state;
      if (type_name.empty()) {
         error("cannot generate bitpack-read function with name %<%s%>: no bitstream state type was ever specified", func_name);
         return;
      }
      auto node = identifier_global_tag(get_identifier(type_name.c_str()));
      if (node == NULL_TREE) {
         error("cannot generate bitpack-read function with name %<%s%>: failed to find a bitstream state type with identifier %<%s%>", func_name, type_name.c_str());
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
            error("cannot generate bitpack-read function with name %<%s%>: failed to find a bitstream state type with identifier %<%s%> (a declaration with that name exists but is not a struct type)", func_name, type_name.c_str());
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
   
   if (options.sectors.count == 1) {
      size_t max_decl_count = std::numeric_limits<size_t>::max();
      //
      // TODO: Pull the max variable count out of the appropriate layout item in 
      //       the global bitpack options.
      //
      
      size_t i = 0;
      for(auto& data_id : request.data_identifiers) {
         if (i++ > max_decl_count)
            break;
         
         auto decl_tree = lookup_name(get_identifier(data_id.c_str()));
         if (decl_tree == NULL_TREE) {
            error("cannot generate bitpack-read function with name %<%s%>: no declaration found for data item %<%s%>", func_name, data_id.c_str());
            return;
         }
         if (TREE_CODE(decl_tree) != VAR_DECL) {
            error("cannot generate bitpack-read function with name %<%s%>: declaration found for data item %<%s%> is not a variable declaration", func_name, data_id.c_str());
            return;
         }
         
         auto object_decl = gw::decl::variable::from_untyped(decl_tree);
         //
         // TODO: generate code
         //
      }
   } else {
      assert(options.sectors.count > 1);
      
      std::vector<gw::expr::call> calls;
      for(size_t i = 0; i < options.sectors.count; ++i) {
         //
         // TODO: generate one function per sector, and call them. only generate 
         //       functions for sectors that aren't empty.
         //
      }
      
      auto root_cond = gw::expr::ternary::from_untyped(NULL_TREE);
      auto prev_cond = gw::expr::ternary::from_untyped(NULL_TREE);
      for(size_t i = 0; i < calls.size(); ++i) {
         auto  cond = gw::expr::ternary(
            types.t_void,
            func_decl.nth_parameter(1).as_value().cmp_is_equal(
               gw::expr::integer_constant(types.t_int, i)
            ),
            calls[i],
            gw::expr::base::from_untyped(NULL_TREE)
         );
         if (prev_cond) {
            prev_cond.set_false_branch(cond);
         } else {
            root_cond = cond;
         }
         prev_cond = cond;
      }
      if (!root_cond.empty()) {
         statements.append(root_cond);
      }
   }
   
   func_dst.set_root_block(root_block);
}