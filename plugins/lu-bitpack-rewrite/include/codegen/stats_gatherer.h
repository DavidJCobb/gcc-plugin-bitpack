#pragma once

// GCC headers fuck up string-related STL identifiers that <memory> depends on.
// `serialization_item` needs <memory> to have been included before the fuckup 
// has happened.
#include <memory>

#include <unordered_map>
#include <vector>
#include "gcc_wrappers/type/base.h"
#include "codegen/serialization_item.h"

namespace codegen {
   class decl_descriptor;
}

namespace codegen {
   class stats_gatherer {
      public:
         struct general_stats {
            //
            // Does not include omitted-and-defaulted values.
            //
            struct {
               size_t total = 0;
               std::vector<size_t> by_sector; // includes pieces of split objects
            } count_seen;
            struct {
               size_t unpacked = 0; // bits // even if a value is transformed, this refers to the original type
               size_t packed   = 0; // bits // if a value is transformed, this is the size of the transformed type
            } total_sizes;
         };
         struct type_stats : public general_stats {
            size_t c_align_of = 0; // in bits
            size_t c_size_of  = 0; // in bits
         };
         
         struct sector_stats {
            size_t total_packed_size = 0;
         };
         
      protected:
         struct {
            size_t current_sector = 0;
         } _state;
         
      public:
         std::vector<sector_stats> stats_by_sector;
         std::unordered_map<std::string, general_stats> stats_by_category;
         std::unordered_map<gcc_wrappers::type::base, type_stats> stats_by_type; // keys are un-transformed types
         
      protected:
         void _seen_stats_category_annotation(std::string_view name, const decl_descriptor&, size_t count);
      
         void _gather_members(const decl_descriptor&, size_t array_count = 1);
         void _seen_decl(const decl_descriptor&, size_t count = 1);
         void _seen_more_of_type(gcc_wrappers::type::base non_transformed, const decl_descriptor&, size_t count);
         
      public:
         void gather_from_sectors(const std::vector<std::vector<serialization_item>>& items_by_sector);
   };
}