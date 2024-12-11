#include "gcc_wrappers/environment/c/language.h"
#include <gcc-plugin.h>
#include <c-family/c-common.h>

namespace gcc_wrappers::environment::c {
   extern language current_language() {
      switch (c_language_kind) {
         case clk_c:      return language::c;
         case clk_cxx:    return language::cpp;
         case clk_objc:   return language::objective_c;
         case clk_objcxx: return language::objective_cpp;
      }
      return language::none;
   }
   
   extern bool language_is_cpp_ish() {
      return c_dialect_cxx();
   }
   extern bool language_is_objective() {
      return c_dialect_objc();
   }
}