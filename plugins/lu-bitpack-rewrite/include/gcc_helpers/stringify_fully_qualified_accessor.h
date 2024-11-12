#pragma once
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/value.h"

namespace gcc_helpers {
   extern std::string stringify_fully_qualified_accessor(gcc_wrappers::value);
}