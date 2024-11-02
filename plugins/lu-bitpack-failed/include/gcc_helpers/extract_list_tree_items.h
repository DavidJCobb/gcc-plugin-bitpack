#pragma once
#include <array>
#include <vector>
#include <gcc-plugin.h> 
#include <tree.h>

namespace gcc_helpers {
   inline void extract_list_tree_pairs(tree list, std::vector<tree>& dst) {
      for(; list != NULL_TREE; list = TREE_CHAIN(list)) {
         dst.push_back(list);
      }
   }
   
   inline void extract_list_tree_values(tree list, std::vector<tree>& dst) {
      for(; list != NULL_TREE; list = TREE_CHAIN(list)) {
         dst.push_back(TREE_VALUE(list));
      }
   }
   
   template<size_t Size>
   void extract_list_tree_pairs(tree list, std::array<tree, Size>& dst) {
      dst = { 0 };
   
      size_t i = 0;
      for(; list != NULL_TREE && i < Size; ++i, list = TREE_CHAIN(list)) {
         dst[i] = list;
      }
   }
   
   template<size_t Size>
   void extract_list_tree_values(tree list, std::array<tree, Size>& dst) {
      dst = { 0 };
   
      size_t i = 0;
      for(; list != NULL_TREE && i < Size; ++i, list = TREE_CHAIN(list)) {
         dst[i] = TREE_VALUE(list);
      }
   }

}