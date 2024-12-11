#include "gcc_wrappers/environment/c/dialect.h"
#include <gcc-plugin.h>
#include <c-family/c-common.h>

namespace gcc_wrappers::environment::c {
   extern dialect current_dialect() {
      if (!flag_isoc94)
         return dialect::c89;
      if (!flag_isoc99)
         return dialect::c89_amendment_1;
      if (!flag_isoc11)
         return dialect::c99;
      if (!flag_isoc23)
         return dialect::c11;
      return dialect::c23;
   }
}