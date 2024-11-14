#pragma once
#include <string>
#include <vector>
#include "codegen/array_access_info.h"
#include "codegen/decl_pair.h"

namespace codegen {
   //
   // Used by instruction tree nodes.
   //
   class decl_described_item {
      public:
         struct segment {
            public:
               decl_pair desc;
               std::vector<array_access_info> array_accesses;
            
            public:
               const bitpacking::data_options::computed& options() const noexcept;
               
               size_t size_in_bits() const noexcept;
               
               bool is_array() const;
               size_t array_extent() const;
               
            public:
               bool operator==(const segment&) const noexcept = default;
         };
         
      public:
         std::vector<segment> segments;
         
         const decl_descriptor& descriptor() const noexcept;
         const bitpacking::data_options::computed& options() const noexcept;
   };
}