#include "handle_kv_string.h"
#include <iostream>
#include <charconv> // std::from_chars
#include "lu/strings/trim.h"

namespace _impl {
   //
   // Parse a string of a form like `key=value,key,key=value`, checking if the keys 
   // are present in a list of known parameters and if so, invoking the parameters' 
   // handlers as appropriate.
   //
   // TODO: Report detailed error info to the caller so they can display error messages 
   // via GCC error(). Maybe throw an exception?
   //
   bool handle_kv_string(std::string_view data, const kv_string_param* params, size_t param_count) {
      if (data.empty()) {
         return true;
      }
      size_t i    = -1;
      size_t prev;
      do {
         std::string_view subarg;
         
         prev = i;
         i    = data.find_first_of(",=", prev + 1);
         if (i == std::string::npos) {
            subarg = data.substr(prev + 1);
         } else {
            subarg = data.substr(prev + 1, i - (prev + 1));
         }
         lu::strings::trim_in_place(subarg);
         for(size_t _pi = 0; _pi < param_count; ++_pi) {
            auto& p = params[_pi];
            
            if (subarg != p.name)
               continue;
            
            if (p.has_param) {
               if (i == std::string::npos || data[i] != '=') {
                  std::cerr << "ERROR: key is missing a value (seen: ";
                  std::cerr << subarg;
                  std::cerr << "...)\n";
                  return false; // TODO: find a way to signal what error occurred
               }
               std::string_view value;
               
               size_t j = data.find_first_of(",=", i + 1);
               if (j == std::string::npos) {
                  value = data.substr(i + 1);
                  i = data.size();
                  lu::strings::trim_in_place(value);
               } else {
                  value = data.substr(i + 1, j - (i + 1));
                  i = j;
                  lu::strings::trim_in_place(value);
                  if (data[j] == '=') {
                     std::cerr << "ERROR: `key=value=value` isn't valid (seen: ";
                     std::cerr << subarg;
                     std::cerr << '=';
                     std::cerr << value;
                     std::cerr << '=';
                     std::cerr << "...)\n";
                     return false;
                  }
               }
               
               int value_int = 0;
               if (p.int_param) {
                  auto conv = std::from_chars(
                     value.data(),
                     value.data() + value.size(),
                     value_int
                  );
                  if (conv.ec != std::errc{}) {
                     std::cerr << "ERROR: value is not a number (seen: ";
                     std::cerr << subarg;
                     std::cerr << '=';
                     std::cerr << value;
                     std::cerr << ")\n";
                     return false;
                  }
                  if (conv.ptr < value.data() + value.size()) {
                     std::cerr << "ERROR: unexpected non-whitespace content after number (seen: ";
                     std::cerr << subarg;
                     std::cerr << '=';
                     std::cerr << value;
                     std::cerr << ")\n";
                     return false;
                  }
               }
               if (!p.handler(value, value_int)) {
                  return false;
               }
            } else {
               if (i != std::string::npos && data[i] == '=') {
                  std::cerr << "ERROR: this key does not allow a value (seen: ";
                  std::cerr << subarg;
                  std::cerr << '=';
                  std::cerr << "...)\n";
                  return false; // TODO: find a way to signal what error occurred
               }
               p.handler("", 0);
            }
            break;
         }
      } while (i != std::string::npos && i + 1 < data.size());
      return true;
   }
}