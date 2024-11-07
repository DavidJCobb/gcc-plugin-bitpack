#include "pragma_handlers/generate_functions.h"
#include <string>
#include <string_view>
#include <vector>

#include <tree.h>
#include <diagnostic.h>

#include "basic_global_state.h"
#include "codegen/sector_functions_generator.h"

#include "gcc_wrappers/builtin_types.h"

// for generating XML output:
#include "xmlgen/sector_xml_generator.h"
#include <fstream>

namespace pragma_handlers {
   extern void generate_functions(cpp_reader* reader) {
   
      // force-create the singleton now, so other code can safely use `get_fast` 
      // to access it.
      gcc_wrappers::builtin_types::get();
      
      std::string read_name;
      std::string save_name;
      std::vector<std::vector<std::string>> identifier_groups;
      
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
            if (key == "read_name" || key == "save_name") {
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
      
      if (read_name.empty()) {
         error_at(start_loc, "%<#pragma lu_bitpack generate_functions%>: missing the %<read_name%> key (the identifier of a function to generate, with signature %<void f(const buffer_byte_type* src, int sector_id)%>)");
         return;
      }
      if (save_name.empty()) {
         error_at(start_loc, "%<#pragma lu_bitpack generate_functions%>: missing the %<save_name%> key (the identifier of a function to generate, with signature %<void f(buffer_byte_type* dst, int sector_id)%>)");
         return;
      }
      
      if (identifier_groups.empty()) {
         warning_at(start_loc, 1, "%<#pragma lu_bitpack generate_functions%>: you did not specify any data to serialize; is this intentional?");
      }
      
      try {
         auto& gs = basic_global_state::get();
         if (gs.any_attributes_missed) {
            error_at(start_loc, "%<#pragma lu_bitpack generate_functions%>: in order to run code generation, you must use %<#pragma lu_bitpack enable%> before the compiler sees any bitpacking attributes");
            return;
         }
         
         gs.global_options.computed.resolve(gs.global_options.requested);
         
         codegen::sector_functions_generator generator(gs.global_options.computed);
         generator.identifiers_to_serialize = identifier_groups;
         generator.top_level_function_names = {
            .read = read_name,
            .save = save_name,
         };
         generator.run();
         
         if (!gs.xml_output_path.empty()) {
            const auto& path = gs.xml_output_path;
            if (path.ends_with(".xml")) {
               xmlgen::sector_xml_generator xml_gen;
               xml_gen.run(generator);
               
               std::ofstream stream(path.c_str());
               assert(!!stream);
               stream << xml_gen.bake();
            }
         }
      } catch (std::runtime_error& ex) {
         std::string message = "%<#pragma lu_bitpack generate_functions%>: ";
         message += ex.what();
         
         error_at(start_loc, message.c_str());
         return;
      }
   }
}