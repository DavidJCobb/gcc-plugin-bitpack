#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/type/base.h"

namespace bitpacking {
   extern void mark_for_invalid_attributes(gcc_wrappers::decl::base);
   extern void mark_for_invalid_attributes(gcc_wrappers::type::base);
}