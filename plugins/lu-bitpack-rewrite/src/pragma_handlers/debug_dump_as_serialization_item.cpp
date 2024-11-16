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
#include "basic_global_state.h"

// re-chunked
#include "codegen/rechunked.h"

// instructions
#include "codegen/rechunked_items_to_instruction_tree.h"
#include "codegen/instruction_tree_stringifier.h"

namespace pragma_handlers { 
   extern void debug_dump_as_serialization_item(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack debug_dump_as_serialization_item";
      
      gcc_wrappers::decl::variable decl;
      size_t levels = std::numeric_limits<size_t>::max();
      
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
            
            token_type = pragma_lex(&data, &loc);
            if (token_type == CPP_NUMBER) {
               if (TREE_CODE(data) != INTEGER_CST) {
                  std::cerr << "note: " << this_pragma_name << ": invalid integer constant for number of levels to dump; treating as 1\n";
                  levels = 1;
               } else {
                  levels = TREE_INT_CST_LOW(data);
               }
            }
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
         
         // can't figure out how to print the offsets properly when dealing with 
         // (potentially nested) conditions on items, so fuck it
         std::cerr << "(note: positions shown will be inaccurate when dealing with conditional items, but sector division should still be accurate)\n";
         
         std::cerr << "\n";
         std::cerr << "   { pos + size} Object path\n";
         
         std::vector<codegen::serialization_item> list;
         list.push_back(item);
         
         std::vector<std::vector<codegen::serialization_item>> sectors;
         
         try {
            sectors = codegen::divide_items_by_sectors(sector_size_in_bytes * 8, list);
            for(size_t i = 0; i < sectors.size(); ++i) {
               std::cerr << "Sector " << i << ":\n";
               
               size_t offset = 0;
               for(size_t j = 0; j < sectors[i].size(); ++j) {
                  auto& item = sectors[i][j];
                  
                  std::cerr << " - ";
                  if (!item.is_omitted) {
                     size_t size = item.size_in_bits();
                     auto   info = lu::strings::printf_string("{%05u+%05u} ", offset, size);
                     std::cerr << info;
                     offset += size;
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
            
            for(size_t i = 0; i < sectors.size(); ++i) {
               std::cerr << "\nSector " << i << ":\n";
               const auto& si_items = sectors[i];
               
               std::vector<codegen::rechunked::item> rc_items;
               rc_items.resize(si_items.size());
               for(size_t j = 0; j < si_items.size(); ++j) {
                  auto& item = rc_items[j];
                  item = codegen::rechunked::item(si_items[j]);
                  
                  using namespace codegen::rechunked;
                  
                  //
                  // Print the re-chunked items as we build them.
                  //
                  
                  auto _print_qd_chunk = [](const chunks::qualified_decl& chunk) {
                     std::cerr << '`';
                     bool first = true;
                     for(auto* desc : chunk.descriptors) {
                        assert(desc != nullptr);
                        if (first) {
                           first = false;
                        } else {
                           std::cerr << '.';
                        }
                        auto decl = desc->decl;
                        if (decl.empty()) {
                           std::cerr << "<empty>";
                        } else {
                           auto name = decl.name();
                           if (name.empty())
                              std::cerr << "<unnamed>";
                           else
                              std::cerr << name;
                        }
                        
                     }
                     std::cerr << '`';
                  };
                  auto _print_as_chunk = [](const chunks::array_slice& chunk) {
                     std::cerr << '[';
                     std::cerr << chunk.data.start;
                     if (chunk.data.count != 1) {
                        std::cerr << ':';
                        std::cerr << (chunk.data.start + chunk.data.count);
                     }
                     std::cerr << ']';
                  };
                  auto _print_tf_chunk = [](const chunks::transform& chunk) {
                     std::cerr << '<';
                     bool first = true;
                     for(auto type : chunk.types) {
                        if (first) {
                           first = false;
                        } else {
                           std::cerr << ' ';
                        }
                        std::cerr << "as ";
                        std::cerr << type.name();
                     }
                     std::cerr << '>';
                  };
                  
                  std::cerr << " - ";
                  bool first = true;
                  for(auto& chunk_ptr : item.chunks) {
                     using namespace codegen::rechunked;
                     
                     if (first) {
                        first = false;
                     } else {
                        std::cerr << ' ';
                     }
                     if (auto* casted = chunk_ptr->as<chunks::qualified_decl>()) {
                        _print_qd_chunk(*casted);
                        continue;
                     }
                     if (auto* casted = chunk_ptr->as<chunks::array_slice>()) {
                        _print_as_chunk(*casted);
                        continue;
                     }
                     if (auto* casted = chunk_ptr->as<chunks::transform>()) {
                        _print_tf_chunk(*casted);
                        continue;
                     }
                     if (auto* casted = chunk_ptr->as<chunks::padding>()) {
                        std::cerr << "{pad:";
                        std::cerr << casted->bitcount;
                        std::cerr << '}';
                        continue;
                     }
                     if (auto* casted = chunk_ptr->as<chunks::condition>()) {
                        std::cerr << '(';
                        for(auto& chunk_ptr : casted->lhs.chunks) {
                           if (auto* casted = chunk_ptr->as<chunks::qualified_decl>()) {
                              _print_qd_chunk(*casted);
                           } else if (auto* casted = chunk_ptr->as<chunks::array_slice>()) {
                              _print_as_chunk(*casted);
                           } else if (auto* casted = chunk_ptr->as<chunks::transform>()) {
                              _print_tf_chunk(*casted);
                           }
                           std::cerr << ' ';
                        }
                        std::cerr << "== ";
                        std::cerr << casted->rhs;
                        std::cerr << ')';
                        continue;
                     }
                  }
                  std::cerr << '\n';
               }
               
            }
         
            //
            // Generate node trees.
            //
            
            std::vector<std::unique_ptr<codegen::instructions::base>> sector_trees;
            for(auto& sector : sectors) {
               // redundant
               std::vector<codegen::rechunked::item> rc_items;
               rc_items.resize(sector.size());
               for(size_t i = 0; i < sector.size(); ++i)
                  rc_items[i] = codegen::rechunked::item(sector[i]);
               
               auto  node_ptr = codegen::rechunked_items_to_instruction_tree(rc_items);
               auto* node     = node_ptr.get();
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