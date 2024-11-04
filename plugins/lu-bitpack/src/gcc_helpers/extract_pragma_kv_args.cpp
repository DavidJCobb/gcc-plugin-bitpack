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
         error_at(pragma_arg_start.loc, "missing %<(%> after %<%s%> - ignored", pragma_name);
         return {};
      }
      
      bool seen_any = false;
      do {
         tree       key_data;
         location_t key_loc;
         tree       val_data;
         location_t val_loc;
         
         auto token_type = pragma_lex(&key_data, &key_loc);
         if (seen_any && token_type == CPP_CLOSE_PAREN) {
            //
            // Trailing comma, e.g. foo(a=b, c=d, )
            //
            warning_at(key_loc, OPT_Wpragmas, "trailing comma at end of %<%s%>", pragma_name);
            break;
         }
         if (token_type != CPP_NAME) {
            error_at(key_loc, "expected key for key/value pair, after %<%s%> - ignored", pragma_name);
            return {};
         }
         seen_any = true;
         
         std::string_view name = IDENTIFIER_POINTER(key_data);
         
         struct {
            location_t loc;
            int        type;
            tree       data;
         } token_after_entry;
         
         bool has_value = false;
         {
            location_t loc;
            tree       data;
            token_type = pragma_lex(&data, &loc);
            
            token_after_entry.loc  = loc;
            token_after_entry.data = data;
            token_after_entry.type = token_type;
            
            switch (token_type) {
               case CPP_EQ:
                  has_value = true;
                  break;
               case CPP_COMMA:
                  has_value = false;
                  break;
               case CPP_CLOSE_PAREN:
                  has_value = false;
                  break;
               default:
                  error_at(key_loc, "expected key/value separator %<=%> or pair separator %<,%>, after %<%s%> - ignored", pragma_name);
                  return {};
            }
         }
         pragma_kv_value value;
         if (has_value) {
            token_type = pragma_lex(&val_data, &val_loc);
            switch (token_type) {
               case CPP_NAME:
                  {
                     auto& dst_v = value.emplace<1>();
                     dst_v = IDENTIFIER_POINTER(val_data);
                  }
                  break;
               case CPP_NUMBER:
                  if (TREE_CODE(val_data) != INTEGER_CST) {
                     error_at(key_loc, "invalid integer constant for pragma %<%s%>, key %<%s%> - ignored", pragma_name, name.data());
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
                  error_at(key_loc, "expected a value for pragma %<%s%>, key %<%s%>, but did not find anything recognizable - ignored", pragma_name, name.data());
                  return {};
            }
         }
         dst[std::string(name)] = value;
         if (has_value) {
            token_after_entry.type = pragma_lex(&token_after_entry.data, &token_after_entry.loc);
         }
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
               error_at(token_after_entry.loc, "expected key/value pair separator %<,%> or closing paren, after %<%s%> - ignored", pragma_name);
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