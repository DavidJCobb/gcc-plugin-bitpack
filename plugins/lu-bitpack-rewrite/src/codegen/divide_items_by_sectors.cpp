#include "codegen/divide_items_by_sectors.h"
#include <algorithm>
#include <cassert>
#include "codegen/decl_descriptor.h"
#include "codegen/serialization_item.h"
#include "debugprint.h"

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
         size_t size = sector.size();
         for(size_t i = 1; i < size; ++i) {
            auto& item = sector[i];
            auto& prev = sector[i - 1];
            
            if (item.is_padding())
               continue;
            if (prev.is_padding())
               continue;
            if (item.is_omitted != prev.is_omitted)
               continue;
            if (item.is_defaulted != prev.is_defaulted)
               continue;
            
            //
            // Check if all but the last path segment are identical.
            //
            size_t length = item.segments.size();
            if (length != prev.segments.size())
               continue;
            bool equal_stems = true;
            for(size_t j = 0; j < length - 1; ++j) {
               if (item.segments[j] != prev.segments[j]) {
                  equal_stems = false;
                  break;
               }
            }
            if (!equal_stems)
               continue;
            
            auto& seg_data_i = item.segments.back().as_basic();
            auto& seg_data_p = prev.segments.back().as_basic();
            
            //
            // Check if the last path segment refers to the same DECL.
            //
            if (seg_data_i.desc != seg_data_p.desc)
               continue;
            
            //
            // Check if all but the innermost array access are identical.
            //
            auto&  item_aa = seg_data_i.array_accesses;
            auto&  prev_aa = seg_data_p.array_accesses;
            size_t rank    = item_aa.size();
            if (rank != prev_aa.size())
               continue;
            bool equal_outer_ranks = true;
            for(size_t j = 0; j < rank - 1; ++j) {
               if (item_aa[j] != prev_aa[j]) {
                  equal_outer_ranks = false;
                  break;
               }
            }
            if (!equal_outer_ranks)
               continue;
            
            //
            // Check if the innermost array accesses are contiguous. Merge the two 
            // serialization items if so, keeping `prev` and destroying `item`.
            //
            if (item_aa.back().start == prev_aa.back().start + prev_aa.back().count) {
               prev_aa.back().count += item_aa.back().count;
               sector.erase(sector.begin() + i);
               --i;
               --size;
               //
               // If, as a result of the merge, this array slice now represents the 
               // entire array, then ditch the innermost array access. For example, 
               // given `int foo[3][2]`, an access like `foo[0][0:2]` will simplify 
               // to `foo[0]`.
               //
               auto& last_aa = prev_aa.back();
               if (last_aa.start == 0) {
                  auto extent = seg_data_p.desc->array.extents[seg_data_p.array_accesses.size() - 1];
                  if (last_aa.count == extent) {
                     prev_aa.resize(prev_aa.size() - 1);
                  }
               }
               
               continue;
            }
            // Else, not mergeable.
         }
      }
      
      //
      // POST-PROCESS: FORCE-EXPAND UNIONS, ANONYMOUS STRUCTS, AND ARRAYS THEREOF
      //
      {
         bool any_changes_made = false;
         do {
            for(auto& list : dst) {
               auto size = list.size();
               for(size_t i = 0; i < size; ++i) {
                  auto& item = list[i];
                  if (item.is_padding())
                     continue;
                  
                  assert(!item.segments.empty());
                  assert(item.segments.back().is_basic());
                  
                  auto& back = item.segments.back().as_basic();
                  auto& desc = item.descriptor();
                  auto  type = desc.types.serialized;
                  if (!type.is_union()) {
                     if (!type.is_record())
                        continue;
                     if (!type.name().empty())
                        continue;
                  }
                  //
                  // This is either a union, an anonymous struct, or an array of either. 
                  // If it's an array, then expand it to the innermost slice.
                  //
                  while (back.array_accesses.size() < desc.array.extents.size()) {
                     size_t rank   = back.array_accesses.size();
                     size_t extent = desc.array.extents[rank];
                     auto&  access = back.array_accesses.emplace_back();
                     access.start = 0;
                     access.count = extent;
                  }
                  //
                  // Now expand the item.
                  //
                  auto   expanded       = item.expanded();
                  size_t expanded_count = expanded.size();
                  
                  std::vector<serialization_item> spliced;
                  spliced.resize(size - 1 + expanded_count);
                  std::copy(
                     list.begin(),
                     list.begin() + i, // copy from [0, i)
                     spliced.begin()
                  );
                  std::copy(
                     expanded.begin(),
                     expanded.end(),
                     spliced.begin() + i // copy to [i, i + e)
                  );
                  if (i + 1 < list.size()) {
                     std::copy(
                        list.begin() + i + 1, // copy from [i + 1, *)
                        list.end(),
                        spliced.begin() + i + expanded_count - 1
                     );
                  }
                  list = spliced;
                  --i;
               }
            }
         } while (any_changes_made);
      }
      
      return dst;
   }
}