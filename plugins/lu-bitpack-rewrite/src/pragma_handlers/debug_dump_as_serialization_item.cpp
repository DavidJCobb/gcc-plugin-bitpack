#include "pragma_handlers/debug_dump_identifier.h"
#include "lu/strings/printf_string.h"
#include <iostream>
#include <limits>
#include <string_view>

#include "gcc_wrappers/decl/variable.h"
#include <c-family/c-common.h> // lookup_name

#include "codegen/decl_descriptor.h"
#include "codegen/serialization_item.h"

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
      item.segments.emplace_back().desc = &descriptor;
      
      std::cerr << "Dumping serialization items for " << decl.name() << "...\n\n";
      
      auto _dump = [](const codegen::serialization_item& item) {
         auto _impl = [](size_t depth, const codegen::serialization_item& item, const auto& recurse) -> void {
            std::string indent(depth * 3, ' ');
            
            std::cerr << indent;
            std::cerr << " - ";
            if (!item.flags.omitted) {
               std::string bits = lu::strings::printf_string("{%05u bits} ", item.size_in_bits());
               std::cerr << bits;
            }
            if (item.flags.defaulted)
               std::cerr << "<D> ";
            if (item.flags.omitted)
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
   }
}