#include "codegen/rechunked/item_to_string.h"
#include <inttypes.h> // PRIdMAX
#include "lu/strings/printf_string.h"
#include "codegen/decl_descriptor.h"
#include "codegen/rechunked.h"

namespace codegen::rechunked {
   static void _stringify_chunk(std::string& dst, const chunks::qualified_decl& chunk) {
      dst += '`';
      
      bool first = true;
      for(auto* desc : chunk.descriptors) {
         assert(desc != nullptr);
         if (first) {
            first = false;
         } else {
            dst += '.';
         }
         auto decl = desc->decl;
         if (decl.empty()) {
            dst += "<empty>";
         } else {
            auto name = decl.name();
            if (name.empty())
               dst += "<unnamed>";
            else
               dst += name;
         }
         
      }
      
      dst += '`';
   }
   static void _stringify_chunk(std::string& dst, const chunks::array_slice& chunk) {
      dst += '[';
      if (chunk.data.count == 1) {
         dst += lu::strings::printf_string("%u", (int)chunk.data.start);
      } else {
         auto end = chunk.data.start + chunk.data.count;
         dst += lu::strings::printf_string("%u:%u", (int)chunk.data.start, (int)end);
      }
      dst += ']';
   }
   static void _stringify_chunk(std::string& dst, const chunks::transform& chunk) {
      dst += '<';
      bool first = true;
      for(auto type : chunk.types) {
         if (first) {
            first = false;
         } else {
            dst += ' ';
         }
         dst += "as ";
         dst += type.name();
      }
      dst += '>';
   }
   
   extern std::string item_to_string(const item& rech) {
      std::string dst;
      
      bool first = true;
      for(auto& chunk_ptr : rech.chunks) {
         using namespace codegen::rechunked;
         
         if (first) {
            first = false;
         } else {
            dst += ' ';
         }
         if (auto* casted = chunk_ptr->as<chunks::qualified_decl>()) {
            _stringify_chunk(dst, *casted);
            continue;
         }
         if (auto* casted = chunk_ptr->as<chunks::array_slice>()) {
            _stringify_chunk(dst, *casted);
            continue;
         }
         if (auto* casted = chunk_ptr->as<chunks::transform>()) {
            _stringify_chunk(dst, *casted);
            continue;
         }
         if (auto* casted = chunk_ptr->as<chunks::padding>()) {
            dst += lu::strings::printf_string("{pad:%u}", (int)casted->bitcount);
            continue;
         }
         if (auto* casted = chunk_ptr->as<chunks::condition>()) {
            dst += '(';
            for(auto& chunk_ptr : casted->lhs.chunks) {
               if (auto* casted = chunk_ptr->as<chunks::qualified_decl>()) {
                  _stringify_chunk(dst, *casted);
               } else if (auto* casted = chunk_ptr->as<chunks::array_slice>()) {
                  _stringify_chunk(dst, *casted);
               } else if (auto* casted = chunk_ptr->as<chunks::transform>()) {
                  _stringify_chunk(dst, *casted);
               } else if (auto* casted = chunk_ptr->as<chunks::condition>()) {
                  dst += "(illegally nested condition)";
               }
               dst += ' ';
            }
            dst += "== ";
            dst += lu::strings::printf_string("%" PRIdMAX, casted->rhs);
            dst += ')';
            continue;
         }
      }
         
      return dst;
   }
}