#pragma once
#include <memory>
#include <vector>
#include "codegen/instructions/base.h"
#include "codegen/rechunked.h"

namespace codegen {
   extern std::unique_ptr<instructions::container> rechunked_items_to_instruction_tree(
      const std::vector<rechunked::item>&
   );
}