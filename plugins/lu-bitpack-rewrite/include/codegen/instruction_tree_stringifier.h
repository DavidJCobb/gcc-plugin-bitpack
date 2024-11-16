#pragma once
#include "lu/strings/builder.h"
#include <string>

namespace codegen {
   namespace instructions {
      class base;
   }
   
   // for debugging
   class instruction_tree_stringifier {
      public:
         std::string data;
         size_t      depth = 0; // tree node nesting depth
         
      protected:
         void _append_indent();
      
         void _stringify_text(const instructions::base&);
         void _stringify_children(const instructions::base&);
         
      public:
         void stringify(const instructions::base&);
   };
}