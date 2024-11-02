#pragma once
#include <string>
#include <vector>
#include "./serialization_item.h"

namespace codegen {
   struct serialization_item;
}

namespace codegen {
   class sector_generator {
      public:
         struct output {
            std::string header;
            std::string implementation;
            size_t      sector_count = 0;
         };
         
         struct sector {
            std::vector<serialization_item*> items;
         };

      public:
         ~sector_generator();
         
         struct {
            size_t sector_size = 1;
         } options;
         std::vector<tree>   top_level_structs;
         std::vector<sector> items_by_sector; // owned
         size_t out_sector_count = 0;

      protected:
         static constexpr const size_t bytespan_of(size_t bitcount) {
            return (bitcount / 8) + (bitcount % 8 ? 1 : 0);
         }

         std::vector<sector> perform_expansions_and_splits(std::vector<serialization_item*>&) const;

      public:
         void prepare(const std::vector<tree>& structures);

         output generate() const;
   };
}