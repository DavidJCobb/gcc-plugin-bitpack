#pragma once

namespace gcc_wrappers::environment::c {
   enum class dialect {
      c89,
      c89_amendment_1,
      c94 = c89_amendment_1,
      c95 = c89_amendment_1,
      c99,
      c11,
      c23,
   };
   constexpr bool operator<(dialect a, dialect b) {
      return (int)a < (int)b;
   }
   constexpr bool operator>(dialect a, dialect b) {
      return (int)a > (int)b;
   }
   
   // If we are not currently loaded into the C compiler, then the 
   // behavior of this function is undefined.
   extern dialect current_dialect();
}