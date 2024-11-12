#include "codegen/serialization_item.h"
#include <limits>
#include "lu/strings/printf_string.h"
#include "codegen/decl_descriptor.h"

namespace codegen {
   
   //
   // serialization_item::segment
   //

   const bitpacking::data_options::computed& serialization_item::segment::options() const noexcept {
      assert(this->desc != nullptr);
      return this->desc->options;
   }
   
   size_t serialization_item::segment::size_in_bits() const noexcept {
      assert(this->desc != nullptr);
      auto  size    = this->desc->serialized_type_size_in_bits();
      auto& extents = this->desc->array.extents;
      for(size_t i = this->array_accesses.size(); i < extents.size(); ++i) {
         size *= extents[i];
      }
      if (!this->array_accesses.empty()) {
         size *= this->array_accesses.back().count;
      }
      return size;
   }
   
   bool serialization_item::segment::is_array() const {
      assert(this->desc != nullptr);
      auto& desc = *this->desc;
      
      if (this->array_accesses.size() < desc.array.extents.size())
         return true;
      
      return false;
   }
   size_t serialization_item::segment::array_extent() const {
      assert(this->desc != nullptr);
      auto& extents  = this->desc->array.extents;
      auto& accesses = this->array_accesses;
      if (accesses.size() < extents.size()) {
         return extents[accesses.size()];
      }
      return 1;
   }
   
   //
   // serialization_item
   //
   
   const decl_descriptor& serialization_item::descriptor() const noexcept {
      return *this->segments.back().desc;
   }
   const bitpacking::data_options::computed& serialization_item::options() const noexcept {
      return this->segments.back().options();
   }
   
   size_t serialization_item::size_in_bits() const {
      return segments.back().size_in_bits();
   }
   
   bool serialization_item::can_expand() const {
      auto& segm = this->segments.back();
      auto& desc = *segm.desc;
      
      // Is array?
      if (segm.array_accesses.size() < desc.array.extents.size())
         return true;
      if (!segm.array_accesses.empty()) {
         if (segm.array_accesses.back().count > 1)
            return true;
      }
      
      if (desc.options.is_buffer()) {
         //
         // TODO: If we want to allow splitting buffers, we'll need a special syntax 
         // for that akin to `array_accesses`.
         //
         return false;
      }
      
      // Is struct or union?
      auto type = desc.types.serialized;
      if (type.is_record() || type.is_union())
         return true;
      
      //
      // TODO: If we allow splitting strings, we'll need more logic here.
      //
      
      return false;
   }
   
   std::vector<serialization_item> serialization_item::expanded() const {
      auto& segm = this->segments.back();
      auto& desc = *segm.desc;
      
      std::vector<serialization_item> out;
      if (segm.is_array()) {
         auto extent = segm.array_extent();
         if (extent == decl_descriptor::vla_extent) {
            //
            // Cannot meaningfully expand a VLA.
            //
         } else {
            for(size_t i = 0; i < extent; ++i) {
               auto clone = *this;
               clone.segments.back().array_accesses.push_back(array_access_info{
                  .start = i,
                  .count = 1,
               });
               out.push_back(std::move(clone));
            }
         }
         return out;
      }
      
      if (desc.options.is_buffer()) {
         //
         // TODO: If we want to allow splitting buffers, we'll need to handle 
         // that here.
         //
         return out;
      }
      
      auto type = desc.types.serialized;
      if (type.is_record()) {
         for(const auto* m : desc.members_of_serialized()) {
            auto clone = *this;
            clone.flags.defaulted = m->options.default_value_node != NULL_TREE;
            clone.flags.omitted   = m->options.omit_from_bitpacking;
            clone.segments.emplace_back().desc = m;
            out.push_back(std::move(clone));
         }
      } else if (type.is_union()) {
         //
         // TODO: Conditions
         //
         assert(false && "not implemented!");
      }
      
      return out;
   }
   
   std::string serialization_item::to_string() const {
      std::string out;
      
      bool first = true;
      for(auto& segm : this->segments) {
         assert(segm.desc != nullptr);
         auto& desc = *segm.desc;
         
         bool transformed = false;
         if (!segm.is_array()) {
            auto& list = desc.types.transformations;
            if (!list.empty()) {
               transformed = true;
               
               std::string text = "(";
               for(auto type : list) {
                  text += "(as ";
                  text += type.pretty_print();
                  text += ") ";
               }
               out.insert(0, text);
            }
         }
         
         if (!first) {
            out += '.';
         }
         out += desc.decl.name();
         for(auto& access : segm.array_accesses) {
            out += '[';
            if (access.count == 1) {
               out += lu::strings::printf_string("%u", (int)access.start);
            } else {
               out += lu::strings::printf_string("%u:%u", (int)access.start, (int)(access.start + access.count));
            }
            out += ']';
         }
         
         if (transformed) {
            out += ')';
         }
         if (first) {
            first = false;
         }
      }
      
      return out;
   }
   
}