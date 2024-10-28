#pragma once
#include "gcc_wrappers/decl/field.h"
#include "bitpack_options.h"

namespace codegen {
   extern BitpackOptions bitpack_options_of(gcc_wrappers::decl::field);
}