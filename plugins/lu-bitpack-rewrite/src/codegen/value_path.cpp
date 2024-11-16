#include "codegen/value_path.h"

namespace codegen {
   void value_path::access_array_element(decl_pair pair) {
      this->segments.emplace_back().data.emplace<1>() = pair;
   }
   void value_path::access_array_element(size_t n) {
      this->segments.emplace_back().data.emplace<size_t>() = n;
   }
   void value_path::access_member(const decl_descriptor& desc) {
      this->segments.emplace_back().data.emplace<0>() = decl_pair{
         .read = &desc,
         .save = &desc
      };
   }
   
   void value_path::replace(decl_pair pair) {
      this->segments.clear();
      this->segments.emplace_back().data.emplace<0>() = pair;
   }
}