#pragma once
#include "lu/strings/zview.h"
#include "gcc_wrappers/type/base.h"

namespace gcc_wrappers::type {
   extern base_ptr lookup_by_name(const char*);
   extern base_ptr lookup_by_name(lu::strings::zview);
}