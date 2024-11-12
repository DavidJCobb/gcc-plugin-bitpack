#include "gcc_helpers/extract_pragma_kv_args.h"
#include <string_view>
#include <type_traits>
#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-pragma.h>
#include <diagnostic.h>

namespace gcc_helpers {
   extern pragma_kv_set extract_pragma_kv_args(
      const char* pragma_name, // for error reporting
      cpp_reader* reader
   ) {
      pragma_kv_set dst;
      struct {
         location_t loc;
         tree       data;
      } pragma_arg_start;
      
      if (pragma_lex(&pragma_arg_start.data, &pragma_arg_start.loc) != CPP_OPEN_PAREN) {
         error_at(pragma_arg_start.loc, "missing %<(%> after %<%s%>; pragma ignored", pragma_name);
         return {};
      }
      
      bool seen_any = false;
      do {
         std::string_view name;
         {
            tree       data;
            location_t loc;
            
            auto token_type = pragma_lex(&data, &loc);
            if (seen_any && token_type == CPP_CLOSE_PAREN) {
               //
               // Trailing comma, e.g. foo(a=b, c=d, )
               //
               warning_at(loc, OPT_Wpragmas, "trailing comma at end of %<%s%>", pragma_name);
               break;
            }
            if (token_type != CPP_NAME) {
               error_at(loc, "expected key for key/value pair, after %<%s%>; pragma ignored", pragma_name);
               return {};
            }
            seen_any = true;
            
            name = IDENTIFIER_POINTER(data);
         }
         
         struct {
            location_t loc;
            int        type;
            tree       data;
         } token_after_entry;
         //
         token_after_entry.type = pragma_lex(&token_after_entry.data, &token_after_entry.loc);
         
         pragma_kv_value value;
         if (token_after_entry.type == CPP_EQ) {
            tree       val_data;
            location_t val_loc;
            
            auto token_type = pragma_lex(&val_data, &val_loc);
            switch (token_type) {
               case CPP_NAME:
                  {
                     auto& dst_v = value.emplace<1>();
                     dst_v = IDENTIFIER_POINTER(val_data);
                  }
                  break;
               case CPP_NUMBER:
                  if (TREE_CODE(val_data) != INTEGER_CST) {
                     error_at(val_loc, "invalid integer constant for pragma %<%s%>, key %<%s%>; pragma ignored", pragma_name, name.data());
                     return {};
                  }
                  {
                     auto& dst_v = value.emplace<3>();
                     dst_v = (intmax_t)TREE_INT_CST_LOW(val_data);
                  }
                  break;
               case CPP_STRING:
                  {
                     auto& dst_v = value.emplace<2>();
                     dst_v = TREE_STRING_POINTER(val_data);
                  }
                  break;
               default:
                  error_at(val_loc, "expected a value for pragma %<%s%>, key %<%s%>, but did not find anything recognizable; pragma ignored", pragma_name, name.data());
                  return {};
            }
            //
            // Consume comma or close-paren.
            //
            token_after_entry.type = pragma_lex(&token_after_entry.data, &token_after_entry.loc);
         }
         dst[std::string(name)] = value;
         //
         // Advance to next pair, or exit.
         //
         bool stop = false;
         switch (token_after_entry.type) {
            case CPP_COMMA:
               continue;
            case CPP_CLOSE_PAREN:
               stop = true;
               break;
            default:
               if (std::holds_alternative<std::monostate>(value)) {
                  error_at(token_after_entry.loc, "expected key/value separator %<=%>, pair separator %<,%>, or closing paren %<)%>, after key %<%s%> in %<%s%>; pragma ignored", name.data(), pragma_name);
               } else {
                  error_at(token_after_entry.loc, "expected key/value pair separator %<,%> or closing paren, after key %<%s%> in %<%s%>; pragma ignored", name.data(), pragma_name);
               }
               return {};
         }
         if (stop)
            break;
      } while (true);
      
      if (pragma_lex(&pragma_arg_start.data) != CPP_EOF)
         warning(OPT_Wpragmas, "junk at end of %<%s%>", pragma_name);
      
      return dst;
   }
}