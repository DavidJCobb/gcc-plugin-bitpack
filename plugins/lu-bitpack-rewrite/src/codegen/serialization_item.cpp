#include "codegen/serialization_item.h"
#include <cassert>
#include <inttypes.h>
#include <limits>
#include "lu/strings/printf_string.h"
#include "codegen/decl_descriptor.h"
#include "debugprint.h"

namespace codegen {
   void serialization_item::append_segment(const decl_descriptor& desc) {
      auto& segm = this->segments.emplace_back();
      auto& data = segm.data.emplace<basic_segment>();
      data.desc = &desc;
   }
   
   const decl_descriptor& serialization_item::descriptor() const noexcept {
      assert(!this->segments.empty());
      auto& segm = this->segments.back();
      assert(segm.is_basic());
      return *segm.as_basic().desc;
   }
   const bitpacking::data_options::computed& serialization_item::options() const noexcept {
      assert(!this->segments.empty());
      auto& segm = this->segments.back();
      assert(segm.is_basic());
      return segm.as_basic().options();
   }
   
   size_t serialization_item::size_in_bits() const {
      if (this->segments.empty())
         return 0;
      auto& segm = this->segments.back();
      if (segm.is_padding()) {
         return segm.as_padding().bitcount;
      } else if (segm.is_basic()) {
         return segm.as_basic().size_in_bits();
      }
      assert(false && "unhandled segment type");
      return 0;
   }
   size_t serialization_item::single_size_in_bits() const {
      if (this->segments.empty())
         return 0;
      auto& segm = this->segments.back();
      if (segm.is_padding()) {
         return segm.as_padding().bitcount;
      } else if (segm.is_basic()) {
         return segm.as_basic().single_size_in_bits();
      }
      assert(false && "unhandled segment type");
      return 0;
   }
   
   static bool has_omitted_defaulted_members(const decl_descriptor& desc) {
      return desc.is_or_contains_defaulted();
   }
   
   bool serialization_item::affects_output_in_any_way() const {
      if (!this->is_omitted)
         return true;
      if (this->is_defaulted)
         return true;
      
      if (this->segments.empty())
         return false;
      auto& back = this->segments.back();
      if (back.is_padding())
         return false;
      assert(back.is_basic());
      auto& segm = back.as_basic();
      auto& desc = *segm.desc;
      if (has_omitted_defaulted_members(desc))
         return true;
      
      return false;
   }
   
   bool serialization_item::can_expand() const {
      if (this->segments.empty())
         return false;
      auto& back = this->segments.back();
      if (back.is_padding())
         return false;
      assert(back.is_basic());
      
      auto& segm = back.as_basic();
      auto& desc = *segm.desc;
      
      if (this->is_omitted && this->is_defaulted) {
         auto dv = desc.options.default_value_node;
         assert(dv != NULL_TREE);
         if (TREE_CODE(dv) == STRING_CST) {
            //
            // If something is omitted and defaulted, and the default value 
            // is a string, then don't expand the innermost extent.
            //
            size_t rank = desc.array.extents.size();
            auto   type = desc.types.serialized;
            if (rank > 0 && !type.is_array()) {
               --rank;
            }
            if (segm.array_accesses.size() < rank)
               return true;
         }
      }
      
      // Is array?
      if (segm.array_accesses.size() < desc.array.extents.size())
         return true;
      
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
   
   std::vector<serialization_item> serialization_item::_expand_record() const {
      std::vector<serialization_item> out;
      
      bool omitted_uplevel = this->is_omitted;
      
      auto& desc = this->descriptor();
      for(const auto* m : desc.members_of_serialized()) {
         bool is_defaulted = m->options.default_value_node != NULL_TREE;
         bool is_omitted   = m->options.omit_from_bitpacking;
         
         if (is_omitted || omitted_uplevel) {
            if (!is_defaulted) {
               //
               // If a member is omitted and not defaulted, then it should be 
               // excluded UNLESS it is or contains a struct that has defaulted 
               // members. (Structs cannot themselves be defaulted.)
               //
               if (!has_omitted_defaulted_members(*m))
                  continue;
            }
         }
         
         auto clone = *this;
         clone.is_defaulted = is_defaulted;
         clone.is_omitted   = is_omitted || omitted_uplevel;
         
         auto& segm = clone.segments.emplace_back();
         auto& data = segm.data.emplace<basic_segment>();
         data.desc = m;
         
         out.push_back(std::move(clone));
      }
      
      return out;
   }
   
   std::vector<serialization_item> serialization_item::_expand_externally_tagged_union() const {
      const auto& desc = this->descriptor();
      assert(desc.types.serialized.is_union());
      assert(desc.options.is_tagged_union());
      const auto& options = desc.options.tagged_union_options();
      assert(!options.is_internal);
      
      std::vector<serialization_item> out;
      
      size_t max_size = single_size_in_bits();
      //
      // Find the union tag.
      //
      std::vector<basic_segment> path_to_tag;
      {
         size_t size = this->segments.size() - 1;
         for(size_t i = 0; i < size; ++i) {
            auto& segm = this->segments[i];
            assert(segm.is_basic());
            path_to_tag.push_back(segm.as_basic());
         }
         
         bool found = false;
         for(auto* m : path_to_tag.back().desc->members_of_serialized()) {
            if (m->decl.name() == options.tag_identifier) {
               path_to_tag.emplace_back().desc = m;
               found = true;
               break;
            }
         }
         assert(found);
      }
      
      //
      // Expand the union.
      //
      for(const auto* m : desc.members_of_serialized()) {
         const bool is_omitted   = m->options.omit_from_bitpacking;
         const bool is_defaulted = m->options.default_value_node != NULL_TREE;
         if (is_omitted && !is_defaulted) {
            continue;
         }
         assert(m->options.union_member_id.has_value());
         
         condition_type cnd;
         cnd.lhs = path_to_tag;
         cnd.rhs = *m->options.union_member_id;
         
         auto clone = *this;
         clone.is_defaulted = is_defaulted;
         clone.is_omitted   = is_omitted;
         {
            auto& segm = clone.segments.emplace_back();
            auto& data = segm.data.emplace<basic_segment>();
            data.desc      = m;
            segm.condition = cnd;
         }
         out.push_back(clone);
         
         const auto clone_size = clone.size_in_bits();
         if (clone_size < max_size) {
            //
            // Zero-pad so that all union permutations are of the same length.
            //
            serialization_item padding = clone;
            padding.segments.back().data.emplace<padding_segment>().bitcount = max_size - clone_size;
            out.push_back(std::move(padding));
         }
      }
      return out;
   }
   
   std::vector<serialization_item> serialization_item::_expand_internally_tagged_union() const {
      auto& desc = this->descriptor();
      assert(desc.types.serialized.is_union());
      assert(desc.options.is_tagged_union());
      auto& options = desc.options.tagged_union_options();
      assert(options.is_internal);
      
      std::vector<serialization_item> out;
      
      size_t max_size = single_size_in_bits();
      //
      // Create serialization items for the union header; track how many 
      // fields comprise the header, including the tag; and count the 
      // total serialized size in bits of the header.
      //
      const auto members = desc.members_of_serialized();
      size_t bits_prior = 0;
      size_t prior      = 0;
      assert(!members.empty());
      {
         bool found  = false;
         auto nested = members[0]->members_of_serialized();
         for(size_t i = 0; i < nested.size(); ++i, ++prior) {
            const auto* m = nested[i];
            
            const bool is_omitted   = m->options.omit_from_bitpacking;
            const bool is_defaulted = m->options.default_value_node != NULL_TREE;
            if (is_omitted) {
               continue;
            }
            
            auto clone = *this;
            clone.is_defaulted = is_defaulted;
            clone.is_omitted   = is_omitted;
            clone.append_segment(*members[0]);
            clone.append_segment(*nested[i]);
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
      // The last field we generated a serialization item for should be the 
      // union tag.
      //
      std::vector<basic_segment> path_to_tag;
      {
         auto& src = out.back().segments;
         for(auto& segm : src) {
            assert(segm.is_basic());
            path_to_tag.push_back(segm.as_basic());
         }
      }
      max_size -= bits_prior;
      //
      // Now, expand the non-shared members-of-members.
      //
      for(const auto* outer : members) {
         const bool is_omitted   = outer->options.omit_from_bitpacking;
         const bool is_defaulted = outer->options.default_value_node != NULL_TREE;
         
         if (is_omitted && !is_defaulted) {
            continue;
         }
         assert(outer->options.union_member_id.has_value());
         condition_type cnd;
         cnd.lhs = path_to_tag;
         cnd.rhs = *outer->options.union_member_id;
         
         if (is_omitted && is_defaulted) {
            auto clone = *this;
            clone.is_defaulted = true;
            clone.is_omitted   = true;
            {
               auto& segm = clone.segments.emplace_back();
               auto& data = segm.data.emplace<basic_segment>();
               data.desc      = outer;
               segm.condition = cnd;
            }
            out.push_back(clone);
            continue;
         }
         
         const auto list        = outer->members_of_serialized();
         size_t     branch_size = 0;
         for(size_t i = prior + 1; i < list.size(); ++i) {
            const auto* m = list[i];
            
            const bool is_omitted   = m->options.omit_from_bitpacking;
            const bool is_defaulted = m->options.default_value_node != NULL_TREE;
            if (is_omitted && !is_defaulted) {
               continue;
            }
            
            auto clone = *this;
            clone.is_defaulted = is_defaulted;
            clone.is_omitted   = is_omitted;
            {
               auto& segm = clone.segments.emplace_back();
               auto& data = segm.data.emplace<basic_segment>();
               data.desc      = outer;
               segm.condition = cnd;
            }
            {
               auto& segm = clone.segments.emplace_back();
               auto& data = segm.data.emplace<basic_segment>();
               data.desc = m;
               //segm.condition = cnd;
            }
            out.push_back(clone);
            
            branch_size += clone.size_in_bits();
         }
         if (branch_size < max_size) {
            //
            // Zero-pad so that all union permutations are of the same length.
            //
            serialization_item padding = *this;
            {
               auto& segm = padding.segments.emplace_back();
               auto& data = segm.data.emplace<padding_segment>();
               data.bitcount  = max_size - branch_size;
               segm.condition = cnd;
            }
            out.push_back(std::move(padding));
         }
      }
      return out;
   }
   
   std::vector<serialization_item> serialization_item::expanded() const {
      if (!this->can_expand())
         return {};
      
      auto& segm = this->segments.back().as_basic();
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
               clone.segments.back().as_basic().array_accesses.push_back(array_access_info{
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
         return this->_expand_record();
      }
      if (type.is_union()) {
         assert(desc.options.is_tagged_union());
         const auto& options = desc.options.tagged_union_options();
         if (options.is_internal) {
            return this->_expand_internally_tagged_union();
         }
         return this->_expand_externally_tagged_union();
      }
      
      return out;
   }
   
   bool serialization_item::is_padding() const {
      if (this->segments.empty())
         return false;
      return this->segments.back().is_padding();
   }
   bool serialization_item::is_union() const {
      if (this->is_padding())
         return false;
      auto& segm = this->segments.back();
      if (!segm.is_basic())
         return false;
      auto& desc = *segm.as_basic().desc;
      auto  type = desc.types.serialized;
      if (!type.is_union())
         return false;
      assert(desc.options.is_tagged_union());
      return true;
   }
   
   std::string serialization_item::to_string() const {
      if (this->segments.empty()) {
         return "<empty>";
      }
      
      std::string out;
      
      bool first = true;
      for(auto& segm : this->segments) {
         if (first) {
            first = false;
         } else {
            out += '|';
         }
         if (segm.condition.has_value()) {
            out += '(';
            out += (*segm.condition).to_string();
            out += ") ";
         }
         
         if (segm.is_padding()) {
            auto bc = segm.as_padding().bitcount;
            out += lu::strings::printf_string("zero_pad_bitcount(%u)", (int)bc);
            continue;
         }
         
         assert(segm.is_basic());
         auto& data = segm.as_basic();
         assert(data.desc != nullptr);
         auto& desc = *data.desc;
         
         out += data.to_string();
         
         auto& list = desc.types.transformations;
         if (!list.empty()) {
            for(auto type : list) {
               out += " as ";
               out += type.pretty_print();
            }
         }
      }
      
      return out;
   }
}