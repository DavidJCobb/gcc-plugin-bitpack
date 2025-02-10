#pragma once
#include <utility>
#include <vector>
#include "lu/singleton.h"
#include "codegen/serialization_item.h"

class last_generation_result;
class last_generation_result : public lu::singleton<last_generation_result> {
   public:
      using offset_size_pair   = std::pair<size_t, size_t>;
      using sector_offset_info = std::vector<offset_size_pair>;
   
   protected:
      std::vector<std::vector<codegen::serialization_item>> items_by_sector;
      std::vector<std::vector<codegen::serialization_item>> items_by_sector_expanded;
      std::vector<sector_offset_info> offsets_by_sector_expanded;
      
   public:
      size_t sector_count() const noexcept { return this->items_by_sector.size(); }
      const std::vector<std::vector<codegen::serialization_item>>& get_sector_items() const noexcept {
         return this->items_by_sector;
      }
      const std::vector<std::vector<codegen::serialization_item>>& get_sector_expanded_items() const noexcept {
         return this->items_by_sector_expanded;
      }
      const sector_offset_info& get_sector_offsets_expanded(size_t index);
      
      void update(
         const std::vector<std::vector<codegen::serialization_item>>& items_by_sector
      );
};