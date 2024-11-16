#include "lu/strings/printf_string.h"
#include <inttypes.h> // PRIdMAX
#include "codegen/serialization_items/condition.h"

namespace codegen::serialization_items {
   std::string condition_type::to_string() const {
      std::string out;
      
      bool first = true;
      for(auto& segm : this->lhs) {
         if (!first)
            out += '.';
         first = false;
         
         out += segm.to_string();
      }
      out += " == ";
      out += lu::strings::printf_string("%" PRIdMAX, this->rhs);
      
      return out;
   }
}