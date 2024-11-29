#include "lu/strings/printf_string.h"
#include <array>
#include <memory> // GCC headers fuck up string-related STL identifiers that <memory> depends on
#include "pragma_handlers/generate_functions.h"
#include <string>
#include <string_view>
#include <vector>

#include <tree.h>
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier
#include <diagnostic.h>

#include "basic_global_state.h"
#include "codegen/debugging/print_sectored_serialization_items.h"
#include "codegen/debugging/print_sectored_rechunked_items.h"
#include "codegen/instructions/base.h"
#include "codegen/decl_descriptor.h"
#include "codegen/describe_and_check_decl_tree.h"
#include "codegen/divide_items_by_sectors.h"
#include "codegen/expr_pair.h"
#include "codegen/func_pair.h"
#include "codegen/instruction_generation_context.h"
#include "codegen/rechunked.h"
#include "codegen/rechunked_items_to_instruction_tree.h"
#include "codegen/serialization_item.h"
#include "codegen/value_pair.h"
#include "codegen/whole_struct_function_dictionary.h"

#include "gcc_helpers/c/at_file_scope.h"
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/local_block.h"
#include "gcc_wrappers/expr/ternary.h"
#include "gcc_wrappers/type/helpers/make_function_type.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/function.h"
#include "gcc_wrappers/builtin_types.h"
#include "gcc_wrappers/scope.h"
namespace gw {
   using namespace gcc_wrappers;
}

// for generating XML output:
#include "codegen/stats_gatherer.h"
#include "xmlgen/report_generator.h"
#include <fstream>

namespace pragma_handlers {
   // Returns true if valid; false if error.
   static bool _check_and_report_generated_function_name(
      const char*        key,
      const char*        sig,
      const std::string& name
   ) {
      if (name.empty()) {
         error("%<#pragma lu_bitpack generate_functions%>: missing the %<%s%> key (the identifier of a function to generate, with signature %<%s%>)", key, sig);
         return false;
      }
      
      auto id_node = get_identifier(name.c_str());
      auto node    = lookup_name(id_node);
      if (node != NULL_TREE) {
         gw::scope scope;
         if (DECL_P(node)) {
            scope = gw::decl::base::from_untyped(node).context();
         } else if (TYPE_P(node)) {
            //
            // AFAIK in C, types can't be nested.
            //
         } else {
            error("%<#pragma lu_bitpack generate_functions%>: per the %<%s%> key we should generate a function with identifier %qE, but that identifier is already in use by something else", key, id_node);
            return false;
         }
         if (!scope.empty() && !scope.is_file_scope()) {
            //
            // We already report errors if we're not in file scope, so ignore 
            // identifiers from nested scopes.
            //
            return false;
         }
         if (TREE_CODE(node) != FUNCTION_DECL) {
            error("%<#pragma lu_bitpack generate_functions%>: per the %<%s%> key we should generate a function with identifier %qE, but that identifier is already in use by something else", key, id_node);
            return false;
         }
      }
      
      return true;
   }
   
   
   static bool _check_and_report_identifier_to_serialize(const std::string& name) {
      auto ident = get_identifier(name.c_str());
      auto node  = lookup_name(ident);
      if (node == NULL_TREE) {
         error("%<#pragma lu_bitpack generate_functions%>: identifier %qE is not defined", ident);
         return false;
      }
      if (TREE_CODE(node) != VAR_DECL) {
         error("%<#pragma lu_bitpack generate_functions%>: identifier %qE does not name a variable", ident);
         return false;
      }
      return true;
   }
   
   
   static bool _check_and_report_missing_builtin(
      location_t         loc,
      gw::decl::function decl,
      std::string_view   name,
      std::string_view   header
   ) {
      if (!decl.empty())
         return true;
      
      error_at(loc, "%<#pragma lu_bitpack generate_functions%>: unable to find a definition for %<%s%> or %<__builtin_%s%>, which we may need for code generation", name.data(), name.data());
      inform(loc, "%qs would be absent if %s has not been included yet; and %<__builtin_%s%>, used a fallback, would be absent based on command line parameters passed to GCC", name.data(), header.data(), name.data());
      return false;
   }
   
   
   template<size_t ArgCount>
   static bool _check_and_report_builtin_signature(
      location_t         loc,
      gw::decl::function decl,
      gw::type::base     return_type,
      std::array<gw::type::base, ArgCount> args
   ) {
      if (decl.empty())
         return true;
      auto type  = decl.function_type();
      bool valid = true;
      if (type.return_type() != return_type) {
         valid = false;
      } else if (type.is_unprototyped() || type.is_varargs() || type.fixed_argument_count() != ArgCount) {
         valid = false;
      } else {
         for(size_t i = 0; i < ArgCount; ++i) {
            auto arg_type = type.nth_argument_type(i);
            if (arg_type != args[i]) {
               valid = false;
               break;
            }
         }
      }
      if (!valid) {
         error_at(loc, "%<#pragma lu_bitpack generate_functions%>: function %<%s%>, which we may need for code generation, has an unexpected signature", decl.name().data());
         
         {
            std::string str;
            str += return_type.pretty_print();
            str += ' ';
            str += decl.name();
            str += '(';
            for(size_t i = 0; i < ArgCount; ++i) {
               if (i > 0)
                  str += ", ";
               str += args[i].pretty_print();
            }
            str += ')';
            inform(loc, "expected signature was %<%s%>", str.c_str());
         }
         {
            std::string str;
            str += type.return_type().pretty_print();
            str += ' ';
            str += decl.name();
            str += '(';
            if (!type.is_unprototyped()) {
               bool any = false;
               type.for_each_argument_type([&any, &str](gw::type::base arg_type, size_t i) {
                  any = true;
                  if (i > 0)
                     str += ", ";
                  str += arg_type.pretty_print();
               });
               if (type.is_varargs()) {
                  if (any)
                     str += ", ";
                  str += "...";
               }
            }
            str += ')';
            inform(loc, "signature seen is %<%s%>", str.c_str());
            
            if (type.is_unprototyped() && ArgCount == 0) {
              inform(loc, "prior to C23, the syntax %<void f()%> defines an unprototyped (pre-C89) function declaration that can take any amount of arguments, as opposed to %<void f(void)%>, which would define a function that takes no arguments"); 
            }
         }
      }
      return valid;
   }
   
   
   extern void generate_functions(cpp_reader* reader) {
   
      // force-create the singleton now, so other code can safely use `get_fast` 
      // to access it.
      gw::builtin_types::get();
      
      std::string read_name;
      std::string save_name;
      std::vector<std::vector<std::string>> identifier_groups;
      bool do_debug_printing = false;
      
      location_t start_loc;
      
      {
         location_t loc;
         tree       data;
         if (pragma_lex(&data, &loc) != CPP_OPEN_PAREN) {
            error_at(loc, "%<#pragma lu_bitpack generate_functions%>: %<(%> expected");
            return;
         }
         start_loc = loc;
         
         do {
            std::string_view key;
            switch (pragma_lex(&data, &loc)) {
               case CPP_NAME:
                  key = IDENTIFIER_POINTER(data);
                  break;
               case CPP_STRING:
                  key = TREE_STRING_POINTER(data);
                  break;
               default:
                  error_at(loc, "%<#pragma lu_bitpack generate_functions%>: expected key name for key/value pair");
                  return;
            }
            if (pragma_lex(&data, &loc) != CPP_EQ) {
               error_at(loc, "%<#pragma lu_bitpack generate_functions%>: expected key/value separator %<=%> after key %<%s%>", key.data());
               return;
            }
            
            int token;
            //
            // Each branch should end by grabbing the next token after the 
            // last accepted token. Code after the branch acts on the last 
            // grabbed token.
            //
            if (key == "enable_debug_output") {
               int value = 0;
               switch (pragma_lex(&data, &loc)) {
                  case CPP_NAME:
                     {
                        std::string_view name = IDENTIFIER_POINTER(data);
                        if (name == "true") {
                           value = 1;
                        } else if (name == "false") {
                           value = 0;
                        } else {
                           error_at(loc, "%<#pragma lu_bitpack generate_functions%>: expected integer literal or identifiers %<true%> or %<false%> as value for key %<%s%>", key.data());
                           return;
                        }
                     }
                     break;
                  case CPP_NUMBER:
                     if (TREE_CODE(data) == INTEGER_CST) {
                        value = TREE_INT_CST_LOW(data);
                        break;
                     }
                     [[fallthrough]];
                  default:
                     error_at(loc, "%<#pragma lu_bitpack generate_functions%>: expected integer literal or identifiers %<true%> or %<false%> as value for key %<%s%>", key.data());
                     return;
               }
               do_debug_printing = value != 0;
               
               token = pragma_lex(&data, &loc);
            } else if (key == "read_name" || key == "save_name") {
               std::string_view name;
               switch (pragma_lex(&data, &loc)) {
                  case CPP_NAME:
                     name = IDENTIFIER_POINTER(data);
                     break;
                  case CPP_STRING:
                     name = TREE_STRING_POINTER(data);
                     break;
                  default:
                     error_at(loc, "%<#pragma lu_bitpack generate_functions%>: expected function name as value for key %<%s%>", key.data());
                     return;
               }
               if (key == "read_name") {
                  read_name = name;
               } else {
                  save_name = name;
               }
               
               token = pragma_lex(&data, &loc);
            } else if (key == "data") {
               
               auto _store_name = [&data, &identifier_groups]() {
                  std::string id = IDENTIFIER_POINTER(data);
                  
                  if (identifier_groups.empty()) {
                     identifier_groups.resize(1);
                  }
                  identifier_groups.back().push_back(id);
               };
               
               if (pragma_lex(&data, &loc) != CPP_NAME) {
                  error_at(loc, "%<#pragma lu_bitpack generate_functions%>: identifier expected as part of value for key %<%s%>", key.data());
                  return;
               }
               do {
                  _store_name();
                  
                  token = pragma_lex(&data, &loc);
                  if (token == CPP_OR) {
                     //
                     // Split to next group.
                     //
                     if (!identifier_groups.empty())
                        identifier_groups.push_back({});
                     //
                     // Next token.
                     //
                     token = pragma_lex(&data, &loc);
                     if (token != CPP_NAME) {
                        error_at(loc, "%<#pragma lu_bitpack generate_functions%>: identifier expected after %<|%>, as part of value for key %<%s%>", key.data());
                        return;
                     }
                  }
                  if (token == CPP_NAME) {
                     continue;
                  } else if (token == CPP_COMMA || token == CPP_CLOSE_PAREN) {
                     break;
                  } else {
                     error_at(loc, "%<#pragma lu_bitpack generate_functions%>: expected identifier or %<|%> within value for key %<%s%>, or %<,%> to end the value", key.data());
                  }
               } while (true);
               
            } else {
               error_at(loc, "%<#pragma lu_bitpack generate_functions%>: unrecognized key %<%s%>", key.data());
               return;
            }
            //
            // Handle comma and close-paren.
            //
            if (token == CPP_COMMA) {
               continue;
            }
            if (token == CPP_CLOSE_PAREN) {
               break;
            }
            error_at(loc, "%<#pragma lu_bitpack generate_functions%>: expected %<,%> or %<)%> after value for key %<%s%>", key.data());
            return;
         } while (true);
      }
      
      if (identifier_groups.empty()) {
         warning_at(start_loc, 1, "%<#pragma lu_bitpack generate_functions%>: you did not specify any data to serialize; is this intentional?");
      }
      
      bool bad_generated_function_names = false;
      if (!_check_and_report_generated_function_name(
         read_name.c_str(),
         "read_name",
         "void f(const buffer_byte_type* src, int sector_id)"
      )) {
         bad_generated_function_names = true;
      }
      if (!_check_and_report_generated_function_name(
         save_name.c_str(),
         "save_name",
         "void f(buffer_byte_type* src, int sector_id)"
      )) {
         bad_generated_function_names = true;
      }
      if (!gcc_helpers::c::at_file_scope()) {
         //
         // GCC name lookups are relative to the current scope. AFAIK there's 
         // no way to only do lookups within file scope. This means that if we 
         // allow this pragma to be used in any inner scope, the identifiers 
         // that the user specified in the pragma options may match function 
         // locals -- and we'll generate file-scope functions that refer to 
         // another function's locals, causing GCC to crash when compiling the 
         // code we generate.
         //
         error_at(start_loc, "%<#pragma lu_bitpack generate_functions%> must be used at file scope (i.e. outside of any function, struct definition, and similar)");
         return;
      }
      
      auto& gs = basic_global_state::get();
      if (gs.any_attributes_missed) {
         error_at(start_loc, "%<#pragma lu_bitpack generate_functions%>: in order to run code generation, you must use %<#pragma lu_bitpack enable%> before the compiler sees any bitpacking attributes");
         
         auto first = gs.error_locations.first_missed_attribute;
         if (first != UNKNOWN_LOCATION)
            inform(first, "the first seen bitpacking attribute was here");
         
         return;
      }
      if (gs.global_options.computed.invalid) {
         error_at(start_loc, "%<#pragma lu_bitpack generate_functions%>: unable to generate code due to a previous error in %<#pragma lu_bitpack set_options%>");
         return;
      }
      
      bool has_builtins = true;
      bool identifiers_valid = false;
      if (gcc_helpers::c::at_file_scope()) {
         identifiers_valid = true;
         for(auto& group : identifier_groups) {
            for(auto& name : group) {
               if (!_check_and_report_identifier_to_serialize(name))
                  identifiers_valid = false;
            }
         }
         
         gs.pull_builtin_functions();
         if (!_check_and_report_missing_builtin(start_loc, gs.builtin_functions.memcpy, "memcpy", "<string.h>")) {
            has_builtins = false;
         } else {
            const auto& ty = gw::builtin_types::get_fast();
            if (!_check_and_report_builtin_signature(
               start_loc,
               gs.builtin_functions.memcpy,
               ty.void_ptr,
               std::array<gw::type::base, 3>{
                  ty.void_ptr,
                  ty.const_void_ptr,
                  ty.size
               }
            )) {
               has_builtins = false;
            }
         }
         if (!_check_and_report_missing_builtin(start_loc, gs.builtin_functions.memset, "memset", "<string.h>")) {
            has_builtins = false;
         } else {
            const auto& ty = gw::builtin_types::get_fast();
            if (!_check_and_report_builtin_signature(
               start_loc,
               gs.builtin_functions.memset,
               ty.void_ptr,
               std::array<gw::type::base, 3>{
                  ty.void_ptr,
                  ty.basic_int,
                  ty.size
               }
            )) {
               has_builtins = false;
            }
         }
      }
      
      if (bad_generated_function_names || !identifiers_valid || !has_builtins) {
         error_at(start_loc, "%<#pragma lu_bitpack generate_functions%>: aborting codegen");
         return;
      }
      
      auto& decl_dictionary = codegen::decl_dictionary::get();
      
      std::vector<std::vector<codegen::serialization_item>> all_sectors_si;
      {
         size_t sector_size_in_bits = gs.global_options.computed.sectors.size_per * 8;
         
         for(auto& group : identifier_groups) {
            std::vector<codegen::serialization_item> items;
            for(auto& identifier : group) {
               auto ident = get_identifier(identifier.c_str());
               auto node  = lookup_name(ident);
               assert(node != NULL_TREE && TREE_CODE(node) == VAR_DECL);
               
               auto decl = gw::decl::variable::from_untyped(node);
               
               const auto& desc = decl_dictionary.get_or_create_descriptor(decl);
               if (!codegen::describe_and_check_decl_tree(desc)) {
                  error_at(start_loc, "%<#pragma lu_bitpack generate_functions%>: aborting codegen due to invalid options applied to a to-be-serialized value");
                  return;
               }
               
               auto& item = items.emplace_back();
               auto& segm = item.segments.emplace_back();
               auto& data = segm.data.emplace<codegen::serialization_items::basic_segment>();
               data.desc = &desc;
            }
            
            auto sectors = codegen::divide_items_by_sectors(sector_size_in_bits, items);
            for(auto& sector : sectors)
               all_sectors_si.push_back(std::move(sector));
         }
      }
      if (do_debug_printing) {
         codegen::debugging::print_sectored_serialization_items(all_sectors_si);
      }
      std::vector<std::vector<codegen::rechunked::item>> all_sectors_ri;
      for(const auto& sector : all_sectors_si) {
         auto& dst = all_sectors_ri.emplace_back();
         for(const auto& item : sector) {
            dst.emplace_back(item);
         }
      }
      if (do_debug_printing) {
         codegen::debugging::print_sectored_rechunked_items(all_sectors_ri);
      }
      //
      // Generate node trees.
      //
      std::vector<std::unique_ptr<codegen::instructions::base>> instructions_by_sector;
      for(const auto& sector : all_sectors_ri) {
         auto node_ptr = codegen::rechunked_items_to_instruction_tree(sector);
         instructions_by_sector.push_back(std::move(node_ptr));
      }
      
      //
      // Generate functions.
      //
      
      const auto& ty = gw::builtin_types::get_fast();
      struct {
         // void __lu_bitpack_read_sector_0(struct lu_BitstreamState*);
         // void __lu_bitpack_save_sector_0(struct lu_BitstreamState*);
         std::vector<codegen::func_pair> per_sector;
         
         // void __lu_bitpack_read(const buffer_byte_type* src, int sector_id);
         // void __lu_bitpack_save(buffer_byte_type* dst, int sector_id);
         codegen::func_pair top_level;
         
         codegen::whole_struct_function_dictionary whole_struct;
      } functions;
      
      {  // Per-sector
         auto per_sector_function_type = gw::type::make_function_type(
            ty.basic_void,
            // args:
            gs.global_options.computed.types.bitstream_state_ptr
         );
         
         for(size_t i = 0; i < instructions_by_sector.size(); ++i) {
            codegen::func_pair pair;
            {
               auto name = lu::strings::printf_string("__lu_bitpack_read_sector_%u", (int)i);
               pair.read = gw::decl::function(name, per_sector_function_type);
            }
            {
               auto name = lu::strings::printf_string("__lu_bitpack_save_sector_%u", (int)i);
               pair.save = gw::decl::function(name, per_sector_function_type);
            }
            pair.read.as_modifiable().set_result_decl(gw::decl::result(ty.basic_void));
            pair.save.as_modifiable().set_result_decl(gw::decl::result(ty.basic_void));
            
            pair.read.nth_parameter(0).make_used();
            pair.save.nth_parameter(0).make_used();
            
            auto ctxt = codegen::instruction_generation_context(functions.whole_struct);
            ctxt.state_ptr = codegen::value_pair{
               .read = pair.read.nth_parameter(0).as_value(),
               .save = pair.save.nth_parameter(0).as_value(),
            };
            auto expr = instructions_by_sector[i]->generate(ctxt);
            
            if (!expr.read.template is<gw::expr::local_block>()) {
               gw::expr::local_block block;
               if (!expr.read.empty())
                  block.statements().append(expr.read);
               expr.read = block;
            }
            if (!expr.save.template is<gw::expr::local_block>()) {
               gw::expr::local_block block;
               if (!expr.save.empty())
                  block.statements().append(expr.save);
               expr.save = block;
            }
            
            pair.read.set_is_defined_elsewhere(false);
            pair.save.set_is_defined_elsewhere(false);
            {
               auto block_read = expr.read.as<gw::expr::local_block>();
               auto block_save = expr.save.as<gw::expr::local_block>();
               pair.read.as_modifiable().set_root_block(block_read);
               pair.save.as_modifiable().set_root_block(block_save);
            }
            
            // expose these identifiers so we can inspect them with our debug-dump pragmas.
            // (in release builds we'll likely want to remove this, so user code can't call 
            // these functions or otherwise access them directly.)
            pair.read.introduce_to_current_scope();
            pair.save.introduce_to_current_scope();
            functions.per_sector.push_back(pair);
         }
      }
      {  // Top-level
         auto& pair = functions.top_level;
         //
         // Get-or-create the functions; then create their bodies.
         //
         bool can_generate_bodies = true;
         {  // void __lu_bitpack_read_by_sector(const buffer_byte_type* src, int sector_id);
            auto c_name  = read_name.c_str();
            auto id_node = get_identifier(c_name);
            auto node    = lookup_name(id_node);
            if (node != NULL_TREE) {
               assert(TREE_CODE(node) == FUNCTION_DECL);
               pair.read = gw::decl::function::from_untyped(node);
               if (pair.read.has_body()) {
                  error("cannot generate a definition for function %qE, as it already has a definition", id_node);
                  can_generate_bodies = false;
               }
            } else {
               pair.read = gw::decl::function(
                  c_name,
                  gw::type::make_function_type(
                     ty.basic_void,
                     // args:
                     gs.global_options.computed.types.buffer_byte_ptr.remove_pointer().add_const().add_pointer(),
                     ty.basic_int // int sectorID
                  )
               );
               inform(UNKNOWN_LOCATION, "generated function %qE did not already have a declaration; implicit declaration generated", id_node);
            }
         }
         {  // void __lu_bitpack_save_by_sector(buffer_byte_type* dst, int sector_id);
            auto c_name  = save_name.c_str();
            auto id_node = get_identifier(c_name);
            auto node    = lookup_name(id_node);
            if (node != NULL_TREE) {
               assert(TREE_CODE(node) == FUNCTION_DECL);
               pair.save = gw::decl::function::from_untyped(node);
               if (pair.save.has_body()) {
                  error("cannot generate a definition for function %qE, as it already has a definition", id_node);
                  can_generate_bodies = false;
               }
            } else {
               pair.save = gw::decl::function(
                  c_name,
                  gw::type::make_function_type(
                     ty.basic_void,
                     // args:
                     gs.global_options.computed.types.buffer_byte_ptr,
                     ty.basic_int // int sectorID
                  )
               );
               inform(UNKNOWN_LOCATION, "generated function %qE did not already have a declaration; implicit declaration generated", id_node);
            }
         }
         if (!can_generate_bodies)
            return;
         
         size_t sector_count = functions.per_sector.size();
         
         auto _generate = [&gs, &ty, &functions, sector_count](gw::decl::function func, bool for_read) {
            auto func_mod = func.as_modifiable();
            
            gw::expr::local_block root_block;
            gw::statement_list    statements = root_block.statements();
            
            auto state_decl = gw::decl::variable("__lu_bitstream_state", gs.global_options.computed.types.bitstream_state);
            state_decl.make_artificial();
            state_decl.make_used();
            //
            statements.append(state_decl.make_declare_expr());
            
            {  // lu_BitstreamInitialize(&state, dst);
               auto src_arg = func.nth_parameter(0).as_value();
               if (for_read) {
                  auto pointer_type = src_arg.value_type();
                  auto value_type   = pointer_type.remove_pointer();
                  if (value_type.is_const()) {
                     src_arg = src_arg.conversion_sans_bytecode(value_type.remove_const().add_pointer());
                  }
               }
               statements.append(
                  gw::expr::call(
                     gs.global_options.computed.functions.stream_state_init,
                     // args:
                     state_decl.as_value().address_of(), // &state
                     src_arg // dst
                  )
               );
            }
         
            std::vector<gw::expr::call> calls;
            calls.resize(sector_count);
            for(size_t i = 0; i < sector_count; ++i) {
               calls[i] = gw::expr::call(
                  for_read ? functions.per_sector[i].read : functions.per_sector[i].save,
                  // args:
                  state_decl.as_value().address_of()
               );
            }
         
            auto root_cond = gw::expr::ternary::from_untyped(NULL_TREE);
            auto prev_cond = gw::expr::ternary::from_untyped(NULL_TREE);
            for(size_t i = 0; i < calls.size(); ++i) {
               auto  cond = gw::expr::ternary(
                  ty.basic_void,
                  
                  // condition:
                  func.nth_parameter(1).as_value().cmp_is_equal(
                     gw::expr::integer_constant(ty.basic_int, i)
                  ),
                  
                  // branches:
                  calls[i],
                  gw::expr::base::from_untyped(NULL_TREE)
               );
               if (!prev_cond.empty()) {
                  prev_cond.set_false_branch(cond);
               } else {
                  root_cond = cond;
               }
               prev_cond = cond;
            }
            if (!root_cond.empty()) {
               statements.append(root_cond);
            }
            
            func.set_is_defined_elsewhere(false);
            func_mod.set_result_decl(gw::decl::result(ty.basic_void));
            func_mod.set_root_block(root_block);
            
            // expose these identifiers so we can inspect them with our debug-dump pragmas.
            // (in release builds we'll likely want to remove this, so user code can't call 
            // these functions or otherwise access them directly.)
            func.introduce_to_current_scope();
         };
         
         _generate(pair.read, true);
         _generate(pair.save, false);
      }
      
      inform(UNKNOWN_LOCATION, "generated the serialization functions");
      if (!gs.xml_output_path.empty()) {
         const auto& path = gs.xml_output_path;
         if (path.ends_with(".xml")) {
            codegen::stats_gatherer  stats;
            stats.gather_from_sectors(all_sectors_si);
            for(auto& group : identifier_groups) {
               for(auto& identifier : group) {
                  auto ident = get_identifier(identifier.c_str());
                  auto node  = lookup_name(ident);
                  assert(node != NULL_TREE && TREE_CODE(node) == VAR_DECL);
                  auto decl = gw::decl::variable::from_untyped(node);
                  stats.gather_from_top_level(decl);
               }
            }
            
            xmlgen::report_generator xml_gen;
            xml_gen.process(functions.whole_struct);
            for(const auto& node_ptr : instructions_by_sector)
               xml_gen.process(*node_ptr->as<codegen::instructions::container>());
            xml_gen.process(stats);
            
            std::ofstream stream(path.c_str());
            assert(!!stream);
            stream << xml_gen.bake();
         }
      }
   }
}