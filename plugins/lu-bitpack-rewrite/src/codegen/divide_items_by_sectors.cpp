#include "codegen/divide_items_by_sectors.h"
#include <algorithm>
#include <cassert>
#include "codegen/decl_descriptor.h"
#include "codegen/serialization_item.h"
#include "debugprint.h"

#include "codegen/serialization_item_list_ops/fold_sequential_array_elements.h"
#include "codegen/serialization_item_list_ops/force_expand_unions_and_anonymous.h"

namespace codegen {
   static void _debugprint_sectors(const std::vector<std::vector<serialization_item>>& sectors) {
      for (auto& sector : sectors) {
         lu_debugprint("sector:\n");
         for(auto& item : sector) {
            lu_debugprint(" - ");
            lu_debugprint(item.to_string());
            lu_debugprint("\n");
         }
      }
   }
   
   extern std::vector<std::vector<serialization_item>> divide_items_by_sectors(
      size_t sector_size_in_bits,
      std::vector<serialization_item> src
   ) {
      std::vector<std::vector<serialization_item>> dst;
      
      size_t remaining = sector_size_in_bits;
      
      auto _insert_into_sector = [&dst](const serialization_item& item) {
         if (dst.empty()) {
            dst.emplace_back().push_back(item);
            return;
         }
         dst.back().push_back(item);
      };
      auto _to_next_sector = [&dst, &remaining, sector_size_in_bits]() {
         dst.emplace_back();
         remaining = sector_size_in_bits;
      };
      
      size_t size = src.size();
      for(size_t i = 0; i < size; ++i) {
         const auto& item = src[i];
         assert(!item.segments.empty());
         
         if (item.is_omitted) {
            if (item.is_defaulted) {
               _insert_into_sector(item);
            }
            continue;
         }
         
         size_t bitcount = item.size_in_bits();
         if (bitcount <= remaining) {
            //
            // If the entire item can fit, then insert it unmodified.
            //
            _insert_into_sector(item);
            remaining -= bitcount;
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
               _insert_into_sector(a);
               _insert_into_sector(b);
               continue;
            }
            //
            // The item can't fit (or it's a union and we want to always expand it). 
            // Expand it if possible; if not, push it to the next sector (after we 
            // verify that it'll even fit there).
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
         }
         
         if (bitcount > sector_size_in_bits) {
            throw std::runtime_error(std::string("found an element that is too large to fit in any sector: ") + item.to_string());
         }
         
         _to_next_sector();
         --i; // re-process current item
         continue;
      }
      
      //
      // POST-PROCESS: CONSECUTIVE ARRAY ELEMENTS TO ARRAY SLICES
      //
      // Join exploded arrays into array slices wherever possible. For example, 
      // { `foo[0]`, `foo[1]`, `foo[2]` } should join to `foo[0:3]` or, if the 
      // entire array is represented there, just `foo`.
      //
      for(auto& sector : dst) {
         serialization_item_list_ops::fold_sequential_array_elements(sector);
      }
      
      //
      // POST-PROCESS: FORCE-EXPAND UNIONS, ANONYMOUS STRUCTS, AND ARRAYS THEREOF
      //
      for(auto& sector : dst) {
         serialization_item_list_ops::force_expand_unions_and_anonymous(sector);
      }
      
      return dst;
   }
}