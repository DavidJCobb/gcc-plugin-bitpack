#include "handle_struct_type.h"
#include "struct.h"

#include <gcc-plugin.h> 
#include <print-tree.h> // debug_tree

#include <iostream> // debug printing

// debugging
#include <cp/cp-tree.h> // same_type_p
#include "gcc_helpers/for_each_in_list_tree.h"
#include "gcc_helpers/for_each_transitive_typedef.h"
#include "gcc_helpers/extract_string_constant_from_tree.h"
#include "gcc_helpers/type_min_max.h"
#include "gcc_helpers/type_name.h"

extern void handle_struct_type(tree type) {
   Struct s;
   s.from_gcc_tree(type);
   
   std::cerr << "Saw struct: " << s.name << ".\n";
   std::cerr << " - Size in bytes: " << s.bytecount << "\n";
   
   std::cerr << " - Members:\n";
   if (s.members.empty()) {
      std::cerr << "    - <None>\n";
   } else {
      for(const auto& member : s.members) {
         std::cerr << "    - " << member.name << "\n";
         std::cerr << "       - Typename: " << member.type.name << "\n";
         
         std::cerr << "       - Fundamental type: ";
         switch (member.type.fundamental) {
            case fundamental_type::unknown:
               std::cerr << "unknown";
               break;
            case fundamental_type::boolean:
               std::cerr << "boolean";
               break;
            case fundamental_type::enumeration:
               std::cerr << "enumeration";
               break;
            case fundamental_type::integer:
               std::cerr << "integer";
               break;
            case fundamental_type::object:
               std::cerr << "object";
               break;
            case fundamental_type::pointer:
               std::cerr << "pointer";
               break;
            case fundamental_type::reference:
               std::cerr << "reference";
               break;
            case fundamental_type::string:
               std::cerr << "string";
               break;
         }
         std::cerr << '\n';
         if (!member.options.do_not_serialize) {
            switch (member.type.fundamental) {
               default:
                  break;
               case fundamental_type::pointer:
               case fundamental_type::reference:
               case fundamental_type::unknown:
                  std::cerr << "          - WARNING: Bitpacking not supported\n";
                  break;
            }
         }
         
         if (auto* ty = member.type.integral) {
            std::cerr << "       - Integral in the range [";
            std::cerr << ty->min;
            std::cerr << ", ";
            std::cerr << ty->max;
            std::cerr << "], sizeof(T) == ";
            std::cerr << ty->bytecount;
            std::cerr << "\n";
         }
         if (member.type.bitfield_bitcount.has_value()) {
            std::cerr << "       - Is bitfield, size ";
            std::cerr << member.type.bitfield_bitcount.value();
            std::cerr << " bits\n";
         }
         
         if (member.type.is_const) {
            std::cerr << "       - Is const\n";
         }
         if (member.type.fundamental == fundamental_type::enumeration) {
            std::cerr << "       - Enumeration with underlying values in the range [";
            std::cerr << member.type.enum_bounds.min;
            std::cerr << ", ";
            std::cerr << member.type.enum_bounds.max;
            std::cerr << "]\n";
         }
         if (!member.type.array_indices.empty()) {
            std::cerr << "       - Is array";
            for(auto extent : member.type.array_indices) {
               std::cerr << '[';
               std::cerr << extent;
               std::cerr << ']';
            }
            std::cerr << '\n';
         }
         
         std::cerr << "       - Bitpacking options:\n";
         if (member.options.do_not_serialize) {
            std::cerr << "          - Do not serialize\n";
         } else {
            std::cerr << "          - Integral\n";
            {
               auto& opt = member.options.integral.bitcount;
               std::cerr << "             - Bitcount: ";
               if (opt.has_value()) {
                  std::cerr << (size_t)opt.value();
                  std::cerr << " <explicitly set>";
               } else {
                  std::cerr << "<not explicitly set>";
               }
               std::cerr << '\n';
            }
            {
               auto& opt = member.options.integral.min;
               std::cerr << "             - Min value: ";
               if (opt.has_value()) {
                  std::cerr << opt.value();
               } else {
                  std::cerr << "<not explicitly set>";
               }
               std::cerr << '\n';
            }
            {
               auto& opt = member.options.integral.max;
               std::cerr << "             - Max value: ";
               if (opt.has_value()) {
                  std::cerr << opt.value();
               } else {
                  std::cerr << "<not explicitly set>";
               }
               std::cerr << '\n';
            }
            {
               auto& os = member.options.string;
               if (os.length.has_value() || os.null_terminated) {
                  std::cerr << "          - String\n";
                  if (os.length.has_value())
                     std::cerr << "             - Length: " << os.length.value() << "\n";
                  if (os.null_terminated)
                     std::cerr << "             - Null-terminated\n";
               }
            }
         }
         
      }
   }
   
   debug_tree(type);
   
   //
   // TEST: distinguishing `char` from `int8_t` and friends
   //
   {
      std::cerr << " - Char [array [array [...]]] members (debug):\n";
      bool any = false;
      
      gcc_helpers::for_each_in_list_tree(TYPE_FIELDS(type), [&any](tree decl) -> bool {
         if (TREE_CODE(decl) != FIELD_DECL)
            return true;
         
         std::string_view name;
         if (auto d_name = DECL_NAME(decl))
            name = IDENTIFIER_POINTER(d_name);
         if (name.empty())
            return true;
         
         auto decl_type = TREE_TYPE(decl);
         if (TREE_CODE(decl_type) != ARRAY_TYPE)
            return true;
         
         auto value_type = TREE_TYPE(decl_type);
         while (value_type != NULL_TREE && TREE_CODE(value_type) == ARRAY_TYPE)
            value_type = TREE_TYPE(value_type);
         if (value_type == NULL_TREE)
            return true;
         if (TREE_CODE(value_type) != INTEGER_TYPE)
            return true;
         {  // check numeric limits against those of `char` on typical platforms
            if (TYPE_UNSIGNED(value_type))
               return true;
            auto pair = gcc_helpers::type_min_max(value_type);
            if (!pair.first.has_value() || !pair.second.has_value())
               return true;
            auto min = pair.first.value();
            auto max = pair.second.value();
            if (min != -128 || max != 127) // if not a signed-char type
               return true;
         }
         
         bool is_char = false;
         gcc_helpers::for_each_transitive_typedef(value_type, [&is_char](tree type) {
            auto name = gcc_helpers::type_name(type);
            if (name == "int8_t") {
               return false; // stop iterating
            }
            if (name == "char") {
               is_char = true;
               return false; // stop iterating
            }
            return true;
         });
         
         if (is_char) {
            any = true;
            std::cerr << "    - ";
            std::cerr << name;
            auto value_type_name = gcc_helpers::type_name(value_type);
            if (value_type_name == "char") {
               std::cerr << " explicitly as char\n";
            } else {
               std::cerr << " via typedefs:\n";
               gcc_helpers::for_each_transitive_typedef(value_type, [&is_char](tree type) {
                  auto name = gcc_helpers::type_name(type);
                  if (name.empty())
                     name = "<unnamed>";
                  std::cerr << "       - " << name << '\n';
                  
                  if (name == "char")
                     return false; // stop iterating
                  return true;
               });
            }
         }
         
         return true;
      });
      if (!any) {
         std::cerr << "    - <none>\n";
      }
   }
}