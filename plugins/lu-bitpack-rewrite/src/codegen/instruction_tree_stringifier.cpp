#include "codegen/instruction_tree_stringifier.h"
#include <inttypes.h> // PRIdMAX
#include "lu/strings/printf_string.h"
#include "codegen/instructions/base.h"
#include "codegen/instructions/array_slice.h"
#include "codegen/instructions/padding.h"
#include "codegen/instructions/single.h"
#include "codegen/instructions/transform.h"
#include "codegen/instructions/union_switch.h"
#include "codegen/instructions/union_case.h"

namespace codegen {
   void instruction_tree_stringifier::_append_indent() {
      auto indent = std::string(this->depth * 3, ' ');
      this->data += indent;
   }
   
   static std::string _var_decl_to_string(gcc_wrappers::decl::variable decl) {
      if (decl.empty())
         return "<VAR_DECL not yet instantiated>";
      auto name = decl.name();
      if (!name.empty())
         return std::string(name);
      return "<unnamed>"; // TODO: include memory address of DECL
   }
   
   static std::string _value_path_to_string(const value_path& value) {
      std::string out;
      for(size_t i = 0; i < value.segments.size(); ++i) {
         const auto& segm = value.segments[i];
         const auto  pair = segm.descriptor();
         if (segm.is_array_access()) {
            if (std::holds_alternative<size_t>(segm.data)) {
               out += lu::strings::printf_string("[%u]", (int)std::get<size_t>(segm.data));
               continue;
            }
            out += '[';
            if (!pair.read->decl.empty()) {
               auto name = pair.read->decl.name();
               if (name.empty()) {
                  out += "<unnamed>";
               } else {
                  out += name;
               }
            } else {
               out += "<missing descriptor>";
            }
            out += ']';
            continue;
         }
         
         if (i > 0)
            out += '.';
         
         if (!pair.read->decl.empty()) {
            auto name = pair.read->decl.name();
            if (name.empty()) {
               out += "<unnamed>";
            } else {
               out += name;
            }
         } else {
            out += "<missing descriptor>";
         }
      }
      return out;
   }
   
   void instruction_tree_stringifier::_stringify_text(const instructions::base& node) {
      if (const auto* casted = node.as<instructions::single>()) {
         this->_append_indent();
         
         this->data += " - ";
         this->data += _value_path_to_string(casted->value);
         this->data += '\n';
         return;
      }
      if (const auto* casted = node.as<instructions::padding>()) {
         this->_append_indent();
         this->data += lu::strings::printf_string(" - padding, %u bits\n", (int)casted->bitcount);
         return;
      }
      if (const auto* casted = node.as<instructions::transform>()) {
         this->_append_indent();
         this->data += " - `";
         this->data += _value_path_to_string(casted->to_be_transformed_value);
         this->data += "` transformed to type `";
         {
            if (casted->types.empty()) {
               this->data += "<missing>";
            } else {
               auto type = casted->types.back();
               auto name = type.name();
               if (name.empty()) {
                  this->data += "<unnamed>";
               } else {
                  this->data += name;
               }
            }
         }
         this->data += "` as variable `";
         this->data += _var_decl_to_string(casted->transformed.variables.read);
         this->data += "`\n";
         return;
      }
      if (const auto* casted = node.as<instructions::array_slice>()) {
         this->_append_indent();
         
         auto end = casted->array.start + casted->array.count;
         
         this->data += " - array `";
         this->data += _value_path_to_string(casted->array.value);
         this->data += lu::strings::printf_string("` slice [%u, %u)", (int)casted->array.start, (int)end);
         if (casted->array.count != 1) {
            this->data += " with loop counter `";
            this->data += _var_decl_to_string(casted->loop_index.variables.read);
            this->data += '`';
         }
         this->data += '\n';
         return;
      }
      if (node.as<instructions::union_case>()) {
         return;
      }
      if (const auto* casted = node.as<instructions::union_switch>()) {
         this->_append_indent();
         this->data += " - branch on `";
         this->data += _value_path_to_string(casted->condition_operand);
         this->data += "`\n";
         return;
      }
      
      this->_append_indent();
      this->data += " - generic node\n";
   }
   
   void instruction_tree_stringifier::_stringify_children(const instructions::base& node) {
      if (const auto* casted = node.as<instructions::container>()) {
         ++this->depth;
         for(const auto& child_ptr : casted->instructions) {
            this->stringify(*child_ptr);
         }
         --this->depth;
      }
      if (const auto* casted = node.as<instructions::union_switch>()) {
         ++this->depth;
         for(const auto& pair : casted->cases) {
            this->_append_indent();
            this->data += lu::strings::printf_string(" - case %" PRIdMAX ":\n", (int)pair.first);
            
            ++this->depth;
            {
               const auto* node = pair.second.get();
               if (!node) {
                  this->_append_indent();
                  this->data += " - <missing node>\n";
               } else {
                  ++this->depth;
                  for(const auto& child_ptr : node->instructions) {
                     this->stringify(*child_ptr);
                  }
                  --this->depth;
               }
            }
            --this->depth;
         }
         --this->depth;
      }
   }
   
   void instruction_tree_stringifier::stringify(const instructions::base& node) {
      this->_stringify_text(node);
      this->_stringify_children(node);
   }
}