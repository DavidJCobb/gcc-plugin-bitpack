#include "pragma_handlers/debug_dump_identifier.h"
#include "lu/strings/printf_string.h"
#include <iostream>
#include <limits>
#include <string_view>

#include "gcc_wrappers/decl/variable.h"
#include <c-family/c-common.h> // lookup_name

#include "codegen/decl_descriptor.h"
#include "codegen/serialization_item.h"
#include "codegen/divide_items_by_sectors.h"
#include "codegen/serialization_item_list_ops/get_offsets_and_sizes.h"
#include "basic_global_state.h"

// re-chunked
#include "codegen/rechunked.h"
#include "codegen/rechunked/item_to_string.h"

// instructions
#include "codegen/rechunked_items_to_instruction_tree.h"
#include "codegen/instruction_tree_stringifier.h"

namespace pragma_handlers { 
   extern void debug_dump_as_serialization_item(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack debug_dump_as_serialization_item";
      
      gcc_wrappers::decl::variable decl;
      {
         tree identifier = NULL_TREE;
         {
            location_t loc;
            tree       data;
            auto token_type = pragma_lex(&data, &loc);
            if (token_type != CPP_NAME) {
               std::cerr << "error: " << this_pragma_name << ": not a valid identifier\n";
               return;
            }
            identifier = data;
         }
         auto node = lookup_name(identifier);
         if (node == NULL_TREE) {
            std::cerr << "error: " << this_pragma_name << ": identifier " << IDENTIFIER_POINTER(identifier) << " not found\n";
            return;
         }
         if (!DECL_P(node)) {
            std::cerr << "error: " << this_pragma_name << ": identifier " << IDENTIFIER_POINTER(identifier) << " is not a declaration\n";
            return;
         }
         if (TREE_CODE(node) != VAR_DECL) {
            std::cerr << "error: " << this_pragma_name << ": identifier " << IDENTIFIER_POINTER(identifier) << " is not a variable declaration\n";
            return;
         }
         decl = gcc_wrappers::decl::variable::from_untyped(node);
      }
      
      auto& dictionary = codegen::decl_dictionary::get();
      auto& descriptor = dictionary.get_or_create_descriptor(decl);
      
      codegen::serialization_item item;
      {
         auto& segm = item.segments.emplace_back();
         auto& data = segm.data.emplace<codegen::serialization_items::basic_segment>();
         data.desc = &descriptor;
      }
      
      std::cerr << "Dumping serialization items for " << decl.name() << "...\n\n";
      
      auto _dump = [](const codegen::serialization_item& item) {
         auto _impl = [](size_t depth, const codegen::serialization_item& item, const auto& recurse) -> void {
            std::string indent(depth * 3, ' ');
            
            std::cerr << indent;
            std::cerr << " - ";
            if (!item.is_omitted) {
               std::string bits = lu::strings::printf_string("{%05u bits} ", item.size_in_bits());
               std::cerr << bits;
            }
            if (item.is_defaulted)
               std::cerr << "<D> ";
            if (item.is_omitted)
               std::cerr << "<O> ";
            std::cerr << item.to_string();
            std::cerr << '\n';
            
            const auto children = item.expanded();
            for(auto& child : children)
               recurse(depth + 1, child, recurse);
         };
         _impl(0, item, _impl);
      };
      _dump(item);
      std::cerr << "Done.\n";
      
      auto& state = basic_global_state::get();
      if (state.global_options_seen) {
         size_t sector_size_in_bytes = state.global_options.computed.sectors.size_per;
         
         std::cerr << '\n';
         std::cerr << "Dividing by sector size " << sector_size_in_bytes << " bytes (" << (sector_size_in_bytes * 8) << " bits):\n";
         
         std::cerr << "\n";
         std::cerr << "   { pos + size} Object path\n";
         
         std::vector<codegen::serialization_item> list;
         list.push_back(item);
         
         std::vector<std::vector<codegen::serialization_item>> sectors;
         
         try {
            sectors = codegen::divide_items_by_sectors(sector_size_in_bytes * 8, list);
            for(size_t i = 0; i < sectors.size(); ++i) {
               std::cerr << "Sector " << i << ":\n";
               
               auto offset_info = codegen::serialization_item_list_ops::get_offsets_and_sizes(sectors[i]);
               assert(offset_info.size() == sectors[i].size());
               for(size_t j = 0; j < sectors[i].size(); ++j) {
                  auto& item = sectors[i][j];
                  auto& info = offset_info[j];
                  
                  std::cerr << " - ";
                  if (!item.is_omitted) {
                     std::cerr << lu::strings::printf_string("{%05u:%05u} ", info.first, info.second);
                  }
                  if (item.is_defaulted)
                     std::cerr << "<D> ";
                  if (item.is_omitted)
                     std::cerr << "<O> ";
                  std::cerr << item.to_string();
                  std::cerr << '\n';
               }
            }
         } catch (std::runtime_error& ex) {
            std::cerr << "Divide failed:\n   " << ex.what() << '\n';
            return;
         }
         
         if (!sectors.empty()) {
            std::cerr << "\n";
            std::cerr << "Generating re-chunked items...\n";
            
            std::vector<std::vector<codegen::rechunked::item>> rechunked_sectors;
            for(const auto& sector : sectors) {
               auto& dst = rechunked_sectors.emplace_back();
               for(const auto& item : sector) {
                  auto& dst_item = dst.emplace_back();
                  dst_item = codegen::rechunked::item(item);
               }
            }
            //
            // Debugprint.
            //
            for(size_t i = 0; i < rechunked_sectors.size(); ++i) {
               std::cerr << "\nSector " << i << ":\n";
               for(auto& item : rechunked_sectors[i]) {
                  std::cerr << " - ";
                  std::cerr << codegen::rechunked::item_to_string(item);
                  std::cerr << '\n';
               }
            }
         
            //
            // Generate node trees.
            //
            
            std::vector<std::unique_ptr<codegen::instructions::base>> sector_trees;
            for(size_t i = 0; i < rechunked_sectors.size(); ++i) {
               const auto& items = rechunked_sectors[i];
               
               auto node_ptr = codegen::rechunked_items_to_instruction_tree(items);
               sector_trees.push_back(std::move(node_ptr));
            }
            
            std::cerr << "\n";
            std::cerr << "Generating instruction trees...\n";
            for(size_t i = 0; i < sector_trees.size(); ++i) {
               std::cerr << "\nSector " << i << ":\n";
               
               codegen::instruction_tree_stringifier str;
               str.stringify(*sector_trees[i].get());
               std::cerr << str.data;
               std::cerr << '\n';
            }
            
         }
         
         std::cerr << "Done.\n";
      }
   }
}