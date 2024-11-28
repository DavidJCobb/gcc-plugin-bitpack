#include "codegen/stats_gatherer.h"
#include "codegen/decl_descriptor.h"
#include "codegen/serialization_item_list_ops/get_offsets_and_sizes.h"

namespace codegen {
   void stats_gatherer::_seen_stats_category_annotation(std::string_view name, const decl_descriptor& desc, size_t count) {
      auto& info = this->stats_by_category[std::string(name)];
      info.count_seen.total += count;
      {
         auto& list = info.count_seen.by_sector;
         if (list.size() < this->_state.current_sector + 1)
            list.resize(this->_state.current_sector + 1);
         list[this->_state.current_sector] += count;
      }
      
      auto type = desc.types.innermost;
      //
      info.total_sizes.packed   += desc.serialized_type_size_in_bits() * count;
      info.total_sizes.unpacked += type.size_in_bytes() * 8 * count;
   }
   
   void stats_gatherer::_gather_members(const decl_descriptor& desc, size_t array_count) {
      auto type = desc.types.serialized;
      if (!type.is_record() && !type.is_union())
         return;
      for(const auto* memb : desc.members_of_serialized()) {
         size_t memb_count = array_count;
         for(auto extent : memb->array.extents)
            memb_count *= extent;
         
         this->_seen_decl(*memb, memb_count);
         auto type = memb->types.innermost;
         this->_seen_more_of_type(type, *memb, memb_count);
         
         this->_gather_members(*memb, memb_count);
      }
   }
   
   void stats_gatherer::_seen_decl(const decl_descriptor& desc, size_t count) {
      for(const auto& name : desc.options.stat_categories) {
         this->_seen_stats_category_annotation(name, desc, count);
      }
   }
   
   void stats_gatherer::_seen_more_of_type(gcc_wrappers::type::base type, const decl_descriptor& desc, size_t count) {
      if (!type.is_record() && !type.is_union())
         return;
      if (type.name().empty())
         //
         // Don't gather stats for anonymous types.
         //
         return;
      
      auto& info = this->stats_by_type[type];
      if (info.c_size_of == 0) {
         //
         // Newly-discovered type. Fill in this information.
         //
         info.c_align_of = type.alignment_in_bytes();
         info.c_size_of  = type.size_in_bytes();
      }
      info.count_seen.total += count;
      {
         auto& list = info.count_seen.by_sector;
         if (list.size() < this->_state.current_sector + 1)
            list.resize(this->_state.current_sector + 1);
         list[this->_state.current_sector] += count;
      }
      info.total_sizes.packed   += desc.serialized_type_size_in_bits() * count;
      info.total_sizes.unpacked += info.c_size_of * 8 * count;
   }
   
   void stats_gatherer::gather_from_sectors(const std::vector<std::vector<serialization_item>>& items_by_sector) {
      for(const auto& sector : items_by_sector) {
         auto  info = serialization_item_list_ops::get_offsets_and_sizes(sector);
         auto& dst  = this->stats_by_sector.emplace_back();
         if (!info.empty()) {
            dst.total_packed_size = info.back().first + info.back().second;
         }
      }
      
      //
      // We need to count structs even if we don't serialize them directly, 
      // but rather serialize their members.
      //
      using stack_entry = serialization_items::basic_segment;
      std::vector<stack_entry> stack;
      //
      for(const auto& sector : items_by_sector) {
         for(const auto& item : sector) {
            if (item.is_padding())
               continue;
            if (item.is_defaulted && item.is_omitted)
               continue;
            
            if (!stack.empty()) {
               //
               // Check to see if we're accessing a different (potentially 
               // nested) DECL from the previous item.
               //
               size_t common_count = 0;
               size_t end          = (std::min)(stack.size(), item.segments.size());
               for(; common_count < end; ++common_count) {
                  const auto& segm = item.segments[common_count];
                  assert(segm.is_basic());
                  if (stack[common_count] != segm.as_basic())
                     break;
               }
               //
               // Remove any DECLs we're no longer inside of.
               //
               if (stack.size() > common_count)
                  stack.resize(common_count);
            }
            
            size_t count = 1;
            for(const auto& entry : stack) {
               for(const auto& access : entry.array_accesses)
                  count *= access.count;
               
               this->_seen_decl(*entry.desc, count);
            }
            
            while (stack.size() < item.segments.size()) {
               size_t i = stack.size();
               const auto& segm = item.segments[i];
               assert(segm.is_basic());
               const auto& data = segm.as_basic();
               const auto* desc = data.desc;
               assert(desc != nullptr);
               stack.push_back(data);
               //
               // Update stats.
               //
               for(const auto& access : data.array_accesses)
                  count *= access.count;
               {
                  const auto&  ranks = desc->array.extents;
                  const size_t from  = stack.back().array_accesses.size();
                  for(size_t i = from; i < ranks.size(); ++i) {
                     count *= ranks[i];
                  }
               }
               //
               this->_seen_decl(*desc, count);
               auto type = desc->types.innermost;
               this->_seen_more_of_type(type, *desc, count);
            }
            
            if (stack.empty()) {
               //
               // This item is empty.
               //
               continue;
            }
            
            //
            // See if we can expand the trailing segment.
            //
            const auto* desc = stack.back().desc;
            assert(desc != nullptr);
            this->_gather_members(*desc, count);
         }
         ++this->_state.current_sector;
      }
   }
}