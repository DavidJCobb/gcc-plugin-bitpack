#include "parse_generate_request.h"
#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-pragma.h>
#include <diagnostic.h>

extern std::optional<generate_request> parse_generate_request(struct cpp_reader* reader) {
   location_t loc;
   tree       data;
   
   auto token_type = pragma_lex(&data, &loc);
   if (token_type != CPP_OPEN_PAREN) {
      error_at(loc, "expected %<(%> for bitpack codegen pragma");
      return {};
   }
   
   auto start_loc = loc;
   
   generate_request request;
   
   do {
      if (pragma_lex(&data, &loc) != CPP_NAME) {
         error_at(loc, "expected key/value pair key for bitpack codegen pragma");
         return {};
      }
      std::string name = IDENTIFIER_POINTER(data);
      
      if (pragma_lex(&data, &loc) != CPP_EQ) {
         error_at(loc, "expected key/value separator %<=%> for bitpack codegen pragma");
         return {};
      }
      
      if (name == "function_identifier") {
         if (pragma_lex(&data, &loc) != CPP_NAME) {
            error_at(loc, "expected variable identifier for key %<function_identifier%> in for bitpack codegen pragma");
            return {};
         }
         request.generated_function_identifier = IDENTIFIER_POINTER(data);
      } else if (name == "data") {
         token_type = pragma_lex(&data, &loc);
         if (token_type == CPP_NAME) {
            request.data_identifiers.push_back(IDENTIFIER_POINTER(data));
         } else if (token_type == CPP_OPEN_PAREN) {
            do {
               if (pragma_lex(&data, &loc) == CPP_NAME) {
                  request.data_identifiers.push_back(IDENTIFIER_POINTER(data));
               } else {
                  error_at(loc, "expected variable identifier within parenthesized list, for key %<data%> in bitpack codegen pragma");
                  return {};
               }
               bool done = false;
               switch (pragma_lex(&data, &loc)) {
                  case CPP_COMMA:
                     continue;
                  case CPP_CLOSE_PAREN:
                     done = true;
                     break;
                  default:
                     error_at(loc, "expected variable identifier separator %<,%>, or list end %<)%>, for key %<data%> in bitpack codegen pragma");
                     return {};
               }
               if (done)
                  break;
            } while (true);
         } else {
            error_at(loc, "expected variable identifier or parenthesized list of variable identifiers, for key %<data%> in bitpack codegen pragma");
            return {};
         }
      } else if (name == "buffer") {
         if (pragma_lex(&data, &loc) != CPP_NAME) {
            error_at(loc, "expected variable identifier for key %<buffer%> in for bitpack codegen pragma");
            return {};
         }
         request.buffer_identifier = IDENTIFIER_POINTER(data);
      } else if (name == "sector") {
         if (pragma_lex(&data, &loc) != CPP_NAME) {
            error_at(loc, "expected variable identifier for for key %<sector%> in bitpack codegen pragma");
            return {};
         }
         request.sector_identifier = IDENTIFIER_POINTER(data);
      }
      
      bool done = false;
      switch (pragma_lex(&data, &loc)) {
         case CPP_COMMA:
            continue;
         case CPP_CLOSE_PAREN:
            done = true;
            break;
         default:
            error_at(loc, "expected key/value pair separator %<,%>, or %<)%>, for bitpack codegen pragma");
            return {};
      }
      if (done)
         break;
   } while (true);
   
   if (request.generated_function_identifier.empty()) {
      error_at(start_loc, "no %<function_identifier%> specified, for bitpack codegen pragma");
      return {};
   }
   if (request.buffer_identifier.empty()) {
      error_at(start_loc, "no %<buffer%> specified, for bitpack codegen pragma");
      return {};
   }
   if (request.sector_identifier.empty()) {
      error_at(start_loc, "no %<sector%> specified, for bitpack codegen pragma");
      return {};
   }
   
   return request;
}