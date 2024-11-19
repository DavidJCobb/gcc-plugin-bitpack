#pragma once
#include <vector>
#include "codegen/instructions/base.h"
#include "codegen/decl_descriptor.h"

namespace codegen {
   struct decl_pair {
      const decl_descriptor* read = nullptr;
      const decl_descriptor* save = nullptr;
      
      // needed by some files due to GCC header jank
      constexpr bool operator==(const decl_pair&) const noexcept = default;
   };
}