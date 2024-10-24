#include "handle_struct_type.h"
#include "struct.h"

#include <gcc-plugin.h> 
#include <print-tree.h> // debug_tree

#include <iostream> // debug printing

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
      }
   }
   
   //
   // TODO: Figure out how to walk the members.
   //
   debug_tree(type);
}