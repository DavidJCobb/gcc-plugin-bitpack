#pragma once
#include <vector>
#include <gcc-plugin.h>
#include <tree.h>

namespace codegen {
   struct structure_member;
   struct structure;
}

namespace codegen {
   struct serialization_item {
      public:
         const structure*        struct_definition = nullptr;
         const structure_member* member_definition = nullptr;
         
         std::vector<size_t> array_indices;
      
      public:
         size_t bitcount() const;
         [[nodiscard]] std::vector<serialization_item*> expand() const;
         
         static serialization_item for_top_level_struct(const structure&);
         
         bool is_array() const;
         bool arg_is_next_array_sibling(const serialization_item&) const;
   };
}