#include "codegen/divide_items_by_sectors.h"
#include <algorithm> // std::copy, std::min
#include <cassert>
#include "codegen/decl_descriptor.h"

#include "codegen/serialization_item_list_ops/fold_sequential_array_elements.h"
#include "codegen/serialization_item_list_ops/force_expand_unions_and_anonymous.h"
#include "codegen/serialization_item_list_ops/force_expand_omitted_and_defaulted.h"

namespace codegen {
   using sector_type = std::vector<serialization_item>;
   using sector_list = std::vector<sector_type>;
   
   using segment_condition = serialization_items::condition_type;
   
   struct overall_state {
      size_t      bits_per_sector = 0;
      sector_list sectors;
   };
   
   struct branch_state {
      overall_state*    overall;
      segment_condition condition;
      size_t bits_remaining = 0;
      size_t current_sector = 0;
      
      branch_state() {} // needed for std::vector
      branch_state(overall_state& o) : overall(&o), bits_remaining(o.bits_per_sector) {}
      
      void insert(const serialization_item& item) {
         if (overall->sectors.size() <= this->current_sector)
            overall->sectors.resize(this->current_sector + 1);
         overall->sectors[this->current_sector].push_back(item);
         
         if (!item.is_omitted)
            this->bits_remaining -= item.size_in_bits();
      }
      void next() {
         ++this->current_sector;
         this->bits_remaining = overall->bits_per_sector;
      }
      
      constexpr bool is_at_or_ahead_of(const branch_state& o) const noexcept {
         if (this->current_sector > o.current_sector)
            return true;
         if (this->current_sector < o.current_sector)
            return false;
         return this->bits_remaining <= o.bits_remaining;
      }
      
      void catch_up_to(const branch_state& o) {
         assert(o.is_at_or_ahead_of(*this));
         this->bits_remaining = o.bits_remaining;
         this->current_sector = o.current_sector;
      }
   };
   
   extern std::vector<std::vector<serialization_item>> divide_items_by_sectors(
      size_t sector_size_in_bits,
      std::vector<serialization_item> src
   ) {
      overall_state overall;
      overall.bits_per_sector = sector_size_in_bits;
      assert(sector_size_in_bits > 0);
      
      branch_state root{overall};
      std::vector<branch_state> branches;
      
      size_t size = src.size();
      for(size_t i = 0; i < size; ++i) {
         const auto& item = src[i];
         assert(!item.segments.empty());
         
         //
         // Grab a list of all conditions that pertain to this serialization item.
         //
         std::vector<segment_condition> conditions;
         for(auto& segment : item.segments)
            if (segment.condition.has_value())
               conditions.push_back(*segment.condition);
         //
         // Check to see if these conditions differ from those that applied to the 
         // previous item(s).
         //
         if (!branches.empty()) {
            size_t common = 0; // count, not index
            size_t end    = (std::min)(conditions.size(), branches.size());
            for(; common < end; ++common)
               if (conditions[common] != branches[common].condition)
                  break;
            
            while (branches.size() > common) {
               //
               // We are no longer subject to (all of) the conditions we were 
               // previously subject to.
               //
               bool same_lhs = false;
               if (conditions.size() >= branches.size()) {
                  auto& a = branches.back().condition;
                  auto& b = conditions[branches.size() - 1];
                  same_lhs = a.lhs == b.lhs;
               }
               if (same_lhs) {
                  //
                  // If multiple objects have conditions with the same LHS, then 
                  // those items go in the same space.
                  //
                  branches.pop_back();
               } else {
                  //
                  // The LHS has changed, so this item doesn't go into the same 
                  // space as the previous. We need to advance the positions of 
                  // those conditions that we're keeping, to move them past the 
                  // previous item's position.
                  //
                  auto  ended = branches.back();
                  branches.pop_back();
                  auto& still = branches.empty() ? root : branches.back();
                  still.catch_up_to(ended);
               }
            }
         }
         while (conditions.size() > branches.size()) {
            //
            // Conditions have been added or changed; start tracking bit positions 
            // for them.
            //
            size_t j    = branches.size();
            auto&  prev = branches.empty() ? root : branches.back();
            auto&  next = branches.emplace_back(overall);
            next.condition      = conditions[j];
            next.bits_remaining = prev.bits_remaining;
            next.current_sector = prev.current_sector;
         }
         
         //
         // All of the logic above is just for dealing with conditions. The logic 
         // below dictates how we actually try to fit serialization items into the 
         // current sector.
         //
         
         auto& current_branch = branches.empty() ? root : branches.back();
         const size_t remaining = current_branch.bits_remaining;
         
         if (item.is_omitted) {
            //
            // Just because an item is omitted doesn't mean it won't affect the 
            // generated code. For example, an omitted-and-defaulted field needs 
            // to be set to its default value when reading from the bitstream; 
            // and an omitted struct can't be defaulted, but may contain members 
            // that are defaulted and so need the same treatment. We must retain  
            // serialization items for these omitted-and-defaulted values.
            //
            if (!item.affects_output_in_any_way())
               continue;
            current_branch.insert(item);
            continue;
         }
         
         size_t bitcount = item.size_in_bits();
         if (bitcount <= remaining) {
            //
            // If the entire item can fit, then insert it unmodified.
            //
            current_branch.insert(item);
            continue;
         }
         //
         // Else, the item can't fit.
         //
         if (remaining > 0) {
            //
            // If there are any bits left in this sector, see if we can slice 
            // or expand the current item to fit it into those bits.
            //
            if (item.is_padding()) {
               //
               // When multiple permutations of a union have different sizes, 
               // we zero-pad the smaller ones to match the larger ones. This 
               // padding can't be expanded, but it's easy to slice.
               //
               serialization_item a = item;
               serialization_item b = item;
               a.segments.back().as_padding().bitcount  = remaining;
               b.segments.back().as_padding().bitcount -= remaining;
               current_branch.insert(a);
               current_branch.insert(b);
               continue;
            }
            //
            // The item can't fit. Expand it if possible; if not, push it to 
            // the next sector (after we verify that it'll even fit there).
            //
            if (item.can_expand()) {
               auto   expanded       = item.expanded();
               size_t expanded_count = expanded.size();
               expanded.resize(size - i - 1 + expanded_count);
               std::copy(
                  src.begin() + i + 1,
                  src.end(),
                  expanded.begin() + expanded_count
               );
               src  = expanded;
               size = expanded.size();
               i    = -1;
               continue;
            }
            //
            // Cannot expand the item.
            //
            if (bitcount > sector_size_in_bits) {
               throw std::runtime_error(std::string("found an element that is too large to fit in any sector: ") + item.to_string());
            }
         }
         
         current_branch.next();
         --i; // re-process current item
         continue;
      }
      
      //
      // POST-PROCESS: CONSECUTIVE ARRAY ELEMENTS TO ARRAY SLICES
      //
      // Join exploded arrays into array slices wherever possible. For example, 
      // { `foo[0]`, `foo[1]`, `foo[2]` } should join to `foo[0:3]` or, if the 
      // entire array is represented there, just `foo`. This will help us with 
      // generating `for` loops to handle these items.
      //
      for(auto& sector : overall.sectors) {
         serialization_item_list_ops::fold_sequential_array_elements(sector);
      }
      
      //
      // POST-PROCESS: FORCE-EXPAND UNIONS, ANONYMOUS STRUCTS, AND ARRAYS THEREOF
      //
      for(auto& sector : overall.sectors) {
         serialization_item_list_ops::force_expand_unions_and_anonymous(sector);
      }
      
      //
      // POST-PROCESS: FORCE-EXPAND OMITTED-AND_DEFAULTED
      //
      // This will ensure that we properly generate instructions to set default 
      // values. It's important primarily for omitted instances of named structs 
      // that contain defaulted members.
      //
      for(auto& sector : overall.sectors) {
         serialization_item_list_ops::force_expand_omitted_and_defaulted(sector);
      }
      
      return overall.sectors;
   }
}