#pragma once
#include <vector>
#include "codegen/serialization_item.h"

namespace codegen {
   extern std::vector<std::vector<serialization_item>> divide_items_by_sectors(
      size_t sector_size_in_bits,
      std::vector<serialization_item> src
   );
}