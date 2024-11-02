#include "codegen/descriptors.h"

namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace codegen {
   
   //
   // member_descriptor
   //
   
   size_t member_descriptor::size_in_bits() const {
      return this->value_type.size_in_bits();
   }
   
   //
   // member_descriptor_view
   //
   
   member_descriptor_view::member_descriptor_view(const member_descriptor& desc) {
      this->_target = &desc;
   }
   
   bitpacking::member_kind member_descriptor_view::kind() const {
      if (this->is_array())
         return bitpacking::member_kind::array;
      return this->_target->kind;
   }
   gw::type member_descriptor_view::type() const {
      auto type = this->target->type;
      for(size_t i = 0; i < this->_state.array_rank; ++i)
         type = type.remove_array_extent();
      return type;
   }
   gw::type member_descriptor_view::innermost_value_type() const {
      return this->target->value_type;
   }
   
   const bitpacking::member_options::computed& member_descriptor_view::bitpacking_options() {
      return this->target->bitpacking_options;
   }

   size_t member_descriptor_view::size_in_bits() const {
      return this->element_size_in_bits() * this->_state.array_extent;
   }
   size_t member_descriptor_view::element_size_in_bits() const {
      size_t bc = this->target->size_in_bits();
      
      auto& ranks = this->target->array_extents;
      for(size_t i = this->_state.array_rank + 1; i < ranks.size(); ++i) {
         bc *= ranks[i];
      }
      return bc;
   }
   
   bool member_descriptor_view::is_array() const {
      auto& ranks = this->target->array_extents;
      if (ranks.empty())
         return false;
      if (this->_state.array_rank == ranks.size() - 1)
         return false;
      return true;
   }
   size_t member_descriptor_view::array_extent() const {
      return this->_state.array_extent;
   }
   
   member_descriptor_view& member_descriptor_view::descend_into_array() {
      auto& ranks = this->target->array_extents;
      assert(is_array());
      assert(this->array_rank + 1 < ranks.size());
      
      ++this->array_rank;
      this->array_extent = ranks[this->array_rank];
      
      return *this;
   }
   member_descriptor_view member_descriptor_view::descended_into_array() const {
      auto it = *this;
      return it.descend_into_array();
   }
   
   //
   // struct_descriptor
   //
   
   struct_descriptor::struct_descriptor(
      const bitpacking::global_options::computed& global,
      gw::type type
   ) {
      this->type = type;
      type.for_each_field([this, &global](tree raw_decl) {
         auto decl = gw::decl::field::from_untyped(raw_decl);
         
         bitpacking::member_options::requested options;
         options.from_field(decl);
         if (options.omit_from_bitpacking)
            return true; // continue
         
         auto& member = this->members.emplace_back(decl);
         member.kind = options.kind;
         member.type = decl.value_type();
         member.decl = decl;
         if (member.type.is_array()) {
            auto array_type = member_type;
            auto value_type = array_type.array_value_type();
            do {
               if (options.is_explicit_string && value_type == global.types.string_char) {
                  break;
               }
               
               size_t extent;
               {
                  auto extent_opt = array_type.array_extent();
                  assert(extent_opt.has_value && "VLAs not supported here!");
                  extent = *extent_opt;
               }
               member.array_extents.push(extent);
               
               if (!value_type.is_array() {
                  break;
               }
               array_type = value_type;
               value_type = array_type.array_value_type();
            } while (true);
         }
         member.bitpacking_options = options;
      });
      
   }
}