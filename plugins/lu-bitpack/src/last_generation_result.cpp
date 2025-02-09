#include "last_generation_result.h"
#include "codegen/serialization_item_list_ops/get_offsets_and_sizes.h"

const last_generation_result::sector_offset_info& last_generation_result::get_sector_offsets(size_t index) {
   assert(index < this->sector_count());
   if (this->offsets_by_sector.size() > index) {
      auto& info = this->offsets_by_sector[index];
      if (!info.empty())
         return info;
      if (this->items_by_sector[index].empty())
         return info;
   } else {
      this->offsets_by_sector.resize(index + 1);
   }
   auto& info = this->offsets_by_sector[index];
   info = codegen::serialization_item_list_ops::get_offsets_and_sizes(this->items_by_sector[index]);
   return info;
}

void last_generation_result::update(
   const std::vector<std::vector<codegen::serialization_item>>& items_by_sector
) {
   this->items_by_sector = items_by_sector;
   this->offsets_by_sector.clear();
}