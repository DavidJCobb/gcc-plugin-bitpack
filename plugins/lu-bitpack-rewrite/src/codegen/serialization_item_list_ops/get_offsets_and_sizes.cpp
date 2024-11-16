#include "codegen/serialization_item_list_ops/get_offsets_and_sizes.h"
#include <algorithm> // std::copy, std::min
#include <cassert>
#include "codegen/decl_descriptor.h"

namespace codegen::serialization_item_list_ops {
   using offset_size_pair = std::pair<size_t, size_t>;
   
   using segment_condition = serialization_items::condition_type;
   
   struct overall_state {
      std::vector<offset_size_pair> data;
   };
   
   struct branch_state {
      overall_state*    overall;
      segment_condition condition;
      size_t offset = 0;
      
      branch_state() {} // needed for std::vector
      branch_state(overall_state& o) : overall(&o) {}
      
      void insert(const serialization_item& item) {
         size_t size = item.size_in_bits();
         overall->data.push_back({ this->offset, size });
         this->offset += size;
      }
      
      void catch_up_to(const branch_state& o) {
         assert(o.offset >= this->offset);
         this->offset = o.offset;
      }
   };
   
   extern std::vector<offset_size_pair> get_offsets_and_sizes(const std::vector<serialization_item>& items) {
      overall_state overall;
      //
      branch_state root(overall);
      std::vector<branch_state> branches;
      
      //
      // This function uses the same basic logic as `divide_items_by_sectors` for 
      // coping with conditional serialization items. We're just counting sizes 
      // and positions rather than actually moving items around.
      //
      
      for(auto& item : items) {
         std::vector<segment_condition> conditions;
         for(auto& segment : item.segments)
            if (segment.condition.has_value())
               conditions.push_back(*segment.condition);
         //
         if (!branches.empty()) {
            size_t common = 0; // count, not index
            size_t end    = (std::min)(conditions.size(), branches.size());
            for(; common < end; ++common)
               if (conditions[common] != branches[common].condition)
                  break;
            
            while (branches.size() > common) {
               bool same_lhs = false;
               if (conditions.size() >= branches.size()) {
                  auto& a = branches.back().condition;
                  auto& b = conditions[branches.size() - 1];
                  same_lhs = a.lhs == b.lhs;
               }
               if (same_lhs) {
                  branches.pop_back();
               } else {
                  auto  ended = branches.back();
                  branches.pop_back();
                  auto& still = branches.empty() ? root : branches.back();
                  still.catch_up_to(ended);
               }
            }
         }
         while (conditions.size() > branches.size()) {
            size_t j    = branches.size();
            auto&  prev = branches.empty() ? root : branches.back();
            auto&  next = branches.emplace_back(overall);
            next.condition = conditions[j];
            next.offset    = prev.offset;
         }
         
         auto& current_branch = branches.empty() ? root : branches.back();
         
         current_branch.insert(item);
      }
      if (!branches.empty()) {
         for(ssize_t i = branches.size() - 2; i >= 0; --i) {
            branches[i].catch_up_to(branches[i + 1]);
         }
         root.catch_up_to(branches[0]);
      }
      
      return overall.data;
   }
}