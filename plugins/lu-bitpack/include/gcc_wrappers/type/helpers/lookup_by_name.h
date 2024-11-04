#pragma once
#include <string>
#include "gcc_wrappers/type/base.h"

namespace gcc_wrappers::type {
   extern base lookup_by_name(const char*);
   extern base lookup_by_name(const std::string&);
}