#include "xmlgen/write_string_as_text_content.h"

namespace xmlgen {
   extern void write_string_as_text_content(std::string_view src, std::string& dst) {
      size_t i = src.find_first_of("<&]");
      if (i == std::string::npos) {
         dst += src;
         return;
      }
      size_t first = i;
      
      bool requires_escape = false;
      while (i != std::string::npos) {
         if (src[i] != ']') {
            requires_escape = true;
            break;
         }
         if (src.substr(i).starts_with("]]>")) {
            requires_escape = true;
            break;
         }
         i = src.find_first_of("<&]", i + 1);
      }
      if (!requires_escape) {
         dst += src;
         return;
      }
      
      dst += src.substr(0, first);
      
      i = first;
      size_t prev;
      while (i != std::string::npos) {
         switch (src[i]) {
            case '<':
               dst += "&lt;";
               break;
            case '&':
               dst += "&amp;";
               break;
            case ']':
               if (src.substr(i).starts_with("]]>")) {
                  dst += "]]&gt;";
                  prev = i + 3;
                  i    = src.find_first_of("<&]", prev);
                  continue;
               }
               dst += src[i];
               break;
         }
         prev = i + 1;
         i    = src.find_first_of("<&]", prev);
      }
      dst += src.substr(prev);
   }
}