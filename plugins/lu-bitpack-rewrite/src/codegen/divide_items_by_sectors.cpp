#include "codegen/divide_items_by_sectors.h"
#include <algorithm>
#include "codegen/decl_descriptor.h"
#include "codegen/serialization_item.h"

namespace codegen {
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
         
         if (item.flags.omitted) {
            if (item.flags.defaulted) {
               _insert_into_sector(item);
            }
            continue;
         }
         
         size_t bitcount = item.size_in_bits();
         if (item.is_union()) {
            //
            // We want to always expand unions, because we need to handle their 
            // members via conditional branches and that's easiest to set up at 
            // this stage in the process.
            //
            assert(item.can_expand());
         } else {
            if (bitcount <= remaining) {
               //
               // If the entire item can fit, then insert it unmodified.
               //
               _insert_into_sector(item);
               remaining -= bitcount;
               continue;
            }
            //
            // Else, the item can't fit. Slice or expand it as appropriate.
            //
            if (item.flags.padding) {
               //
               // When multiple permutations of a union have different sizes, 
               // we zero-pad the smaller ones to match the larger ones. This 
               // padding can't be expanded, but it's easy to slice.
               //
               serialization_item a = item;
               serialization_item b = item;
               a.padding_size = remaining;
               b.padding_size -= remaining;
               _insert_into_sector(a);
               _insert_into_sector(b);
               continue;
            }
         }
         //
         // The item can't fit (or it's a union and we want to always expand it). 
         // Expand it if possible; if not, push it to the next sector (after we 
         // verify that it'll even fit there).
         //
         if (item.can_expand()) {
            auto expanded = item.expanded();
            
            std::vector<serialization_item> replaced;
            replaced.resize(size - i - 1 + expanded.size());
            std::copy(
               src.begin() + i + 1,
               src.end(),
               replaced.begin()
            );
            std::copy(
               expanded.begin(),
               expanded.end(),
               replaced.begin() + size - i - 1
            );
            src  = replaced;
            size = replaced.size();
            i    = -1;
            continue;
         }
         
         //
         // Cannot expand the item.
         //
         
         if (bitcount > sector_size_in_bits) {
            throw std::runtime_error(std::string("found an element that is too large to fit in any sector: ") + item.to_string());
         }
         
         _to_next_sector();
         --i;
         continue;
      }
      
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
            
            if (item.flags != prev.flags)
               continue;
            if (item.conditions != prev.conditions)
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
            
            //
            // Check if the last path segment refers to the same DECL.
            //
            if (item.segments.back().desc != prev.segments.back().desc)
               continue;
            
            //
            // Check if all but the innermost array access are identical.
            //
            auto&  item_aa = item.segments.back().array_accesses;
            auto&  prev_aa = prev.segments.back().array_accesses;
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
               auto& last_sg = prev.segments.back();
               auto& last_aa = prev_aa.back();
               if (last_aa.start == 0) {
                  auto extent = last_sg.desc->array.extents[last_sg.array_accesses.size() - 1];
                  if (last_aa.count == extent) {
                     prev_aa.resize(prev_aa.size() - 1);
                  }
               }
               
               continue;
            }
            // Else, not mergeable.
         }
      }
      
      return dst;
   }
}