#include "parse_generate_request.h"
#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-pragma.h>

extern std::optional<generate_request> parse_generate_request(struct cpp_reader* reader) {
   location_t loc;
   tree       data;
   
   auto token_type = pragma_lex(&data, &loc);
   if (token_type != CPP_OPEN_PAREN) {
      error_at(loc, "expected %<(%> for bitpack codegen pragma", pragma_name);
      return {};
   }
   
   generate_request request;
   
   do {
      if (pragma_lex(&data, &loc) != CPP_NAME) {
         error_at(loc, "expected key/value pair key for bitpack codegen pragma", pragma_name);
         return {};
      }
      std::string name = IDENTIFIER_POINTER(data);
      
      if (pragma_lex(&data, &loc) != CPP_EQ) {
         error_at(loc, "expected key/value separator %<=%> for bitpack codegen pragma", pragma_name);
         return {};
      }
      
      if (name == "data") {
         token_type = pragma_lex(&data, &loc);
         if (token_type == CPP_NAME) {
            request.data_identifiers.push_back(IDENTIFIER_POINTER(data));
         } else if (token_type == CPP_OPEN_PAREN) {
            do {
               if (pragma_lex(&data, &loc) == CPP_NAME) {
                  request.data_identifiers.push_back(IDENTIFIER_POINTER(data));
               } else {
                  error_at(loc, "expected variable identifier within parenthesized list, for key %<data%> in bitpack codegen pragma", pragma_name);
                  return {};
               }
               bool done = false;
               switch (pragma_lex(&data, &loc)) {
                  case CPP_COMMA:
                     continue;
                  case CPP_CLOSE_PAREN:
                     done = true;
                  default:
                     error_at(loc, "expected variable identifier separator %<,%>, or list end %<)%>, for key %<data%> in bitpack codegen pragma", pragma_name);
                     return {};
               }
               if (done)
                  break;
            } while (true);
         } else {
            error_at(loc, "expected variable identifier or parenthesized list of variable identifiers, for key %<data%> in bitpack codegen pragma", pragma_name);
            return {};
         }
      } else if (name == "buffer") {
         if (pragma_lex(&data, &loc) != CPP_NAME) {
            error_at(loc, "expected variable identifier for key %<buffer%> in for bitpack codegen pragma", pragma_name);
            return {};
         }
         request.buffer_identifier = IDENTIFIER_POINTER(data);
      } else if (name == "sector") {
         if (pragma_lex(&data, &loc) != CPP_NAME) {
            error_at(loc, "expected variable identifier for for key %<sector%> in bitpack codegen pragma", pragma_name);
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
         default:
            error_at(loc, "expected key/value pair separator %<,%>, or %<)%>, for bitpack codegen pragma", pragma_name);
            return {};
      }
      if (done)
         break;
   } while (true);
   
   return request;
}