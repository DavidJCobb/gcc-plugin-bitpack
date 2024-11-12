#include "codegen/serialization_item.h"
#include <cassert>
#include <inttypes.h>
#include <limits>
#include "lu/strings/printf_string.h"
#include "codegen/decl_descriptor.h"
#include "debugprint.h"

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
   
   std::string serialization_item::segment::to_string() const {
      std::string out;
      
      out += this->desc->decl.name();
      for(auto& access : this->array_accesses) {
         out += '[';
         if (access.count == 1) {
            out += lu::strings::printf_string("%u", (int)access.start);
         } else {
            out += lu::strings::printf_string("%u:%u", (int)access.start, (int)(access.start + access.count));
         }
         out += ']';
      }
      
      return out;
   }
   
   //
   // serialization_item::condition
   //
   
   std::string serialization_item::condition::to_string() const {
      std::string out;
      
      bool first = true;
      for(auto& item : this->path) {
         if (!first)
            out += '.';
         first = false;
         
         out += item.to_string();
      }
      out += " == ";
      out += lu::strings::printf_string("%" PRIdMAX, this->value);
      
      return out;
   }
   
   //
   // serialization_item
   //
   
   const decl_descriptor& serialization_item::descriptor() const noexcept {
      assert(!this->flags.padding);
      assert(!this->segments.empty());
      return *this->segments.back().desc;
   }
   const bitpacking::data_options::computed& serialization_item::options() const noexcept {
      assert(!this->flags.padding);
      assert(!this->segments.empty());
      return this->segments.back().options();
   }
   
   size_t serialization_item::size_in_bits() const {
      if (this->flags.padding) {
         assert(this->segments.empty());
         return this->padding_size;
      }
      return segments.back().size_in_bits();
   }
   
   bool serialization_item::can_expand() const {
      if (this->flags.padding) {
         return false;
      }
      
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
      if (this->flags.padding) {
         return {};
      }
      
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
         assert(desc.options.is_tagged_union());
         auto&  options  = desc.options.tagged_union_options();
         size_t max_size = size_in_bits();
         if (options.is_internal) {
            //
            // Internally-tagged union. Start by finding the tag -- and counting 
            // how many members-of-members are shared.
            //
            const auto members = desc.members_of_serialized();
            size_t bits_prior = 0;
            size_t prior      = 0;
            {
               assert(!members.empty());
               bool found  = false;
               auto nested = members[0]->members_of_serialized();
               for(size_t i = 0; i < nested.size(); ++i, ++prior) {
                  const auto* m = nested[i];
                  
                  auto clone = *this;
                  clone.flags.defaulted = m->options.default_value_node != NULL_TREE;
                  clone.flags.omitted   = m->options.omit_from_bitpacking;
                  clone.segments.emplace_back().desc = members[0];
                  clone.segments.emplace_back().desc = nested[i];
                  out.push_back(clone);
                  
                  bits_prior += clone.size_in_bits();
                  
                  if (m->decl.name() == options.tag_identifier) {
                     assert(!m->options.omit_from_bitpacking);
                     found = true;
                     break;
                  }
               }
               assert(found);
            }
            //
            const auto tag_path = out.back().segments;
            max_size -= bits_prior;
            //
            // Now, expand the non-shared members-of-members.
            //
            for(const auto* outer : members) {
               if (outer->options.omit_from_bitpacking && outer->options.default_value_node == NULL_TREE) {
                  continue;
               }
               assert(outer->options.union_member_id.has_value());
               condition cnd;
               cnd.path  = tag_path;
               cnd.value = *outer->options.union_member_id;
               
               if (outer->options.omit_from_bitpacking && outer->options.default_value_node != NULL_TREE) {
                  auto clone = *this;
                  clone.flags.defaulted = true;
                  clone.flags.omitted   = true;
                  clone.segments.emplace_back().desc = outer;
                  clone.conditions.push_back(cnd);
                  out.push_back(clone);
                  continue;
               }
               
               auto   list        = outer->members_of_serialized();
               size_t branch_size = 0;
               for(size_t i = prior + 1; i < list.size(); ++i) {
                  const auto* m = list[i];
                  
                  auto clone = *this;
                  clone.flags.defaulted = m->options.default_value_node != NULL_TREE;
                  clone.flags.omitted   = m->options.omit_from_bitpacking;
                  clone.segments.emplace_back().desc = outer;
                  clone.segments.emplace_back().desc = m;
                  clone.conditions.push_back(cnd);
                  out.push_back(clone);
                  
                  branch_size += clone.size_in_bits();
               }
               if (branch_size < max_size) {
                  //
                  // Zero-pad so that all union permutations are of the same length.
                  //
                  serialization_item pad;
                  pad.flags.padding = true;
                  pad.padding_size  = max_size - branch_size;
                  pad.conditions = this->conditions;
                  pad.conditions.push_back(cnd);
                  out.push_back(std::move(pad));
               }
            }
         } else {
            //
            // Externally-tagged union.
            //
            auto tag_path = this->segments;
            {
               tag_path.resize(tag_path.size() - 1);
               {
                  bool found = false;
                  for(auto* m : tag_path.back().desc->members_of_serialized()) {
                     if (m->decl.name() == options.tag_identifier) {
                        tag_path.emplace_back().desc = m;
                        found = true;
                        break;
                     }
                  }
                  assert(found);
               }
            }
            for(const auto* m : desc.members_of_serialized()) {
               condition cnd;
               cnd.path  = tag_path;
               cnd.value = *m->options.union_member_id;
               
               auto clone = *this;
               clone.flags.defaulted = m->options.default_value_node != NULL_TREE;
               clone.flags.omitted   = m->options.omit_from_bitpacking;
               clone.segments.emplace_back().desc = m;
               clone.conditions.push_back(cnd);
               out.push_back(clone);
               
               const auto clone_size = clone.size_in_bits();
               if (clone_size < max_size) {
                  serialization_item pad;
                  pad.flags.padding = true;
                  pad.padding_size = max_size - clone_size;
                  pad.conditions = this->conditions;
                  pad.conditions.push_back(cnd);
                  out.push_back(std::move(pad));
               }
            }
         }
      }
      
      return out;
   }
   
   bool serialization_item::is_union() const {
      if (this->flags.padding)
         return false;
      auto& segm = this->segments.back();
      auto& desc = *segm.desc;
      auto  type = desc.types.serialized;
      if (!type.is_union())
         return false;
      assert(desc.options.is_tagged_union());
      return true;
   }
   
   std::string serialization_item::to_string() const {
      std::string out;
      
      if (!this->conditions.empty()) {
         out += "if (";
         
         bool first = true;
         for(auto& cnd : this->conditions) {
            if (!first)
               out += " && ";
            first = false;
            
            out += cnd.to_string();
         }
         
         out += ") ";
      }
      
      if (this->flags.padding) {
         out += "zero_pad_bitcount(";
         out += lu::strings::printf_string("%u", (int)this->padding_size);
         out += ")";
         return out;
      }
      
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
         out += segm.to_string();
         
         if (transformed) {
            out += ')';
         }
         if (first) {
            first = false;
         }
      }
      
      return out;
   }
   
   bool serialization_item::conditions_are_a_narrowing_of(const serialization_item& other) {
      if (other.conditions.size() >= this->conditions.size())
         return false;
      
      auto size = other.conditions.size();
      for(size_t i = 0; i < size; ++i) {
         const auto& a = this->conditions[i];
         const auto& b = other.conditions[i];
         if (a != b)
            return false;
      }
      return true;
   }
}