#include "codegen/descriptors.h"
#include "lu/strings/printf_string.h"
#include "bitpacking/global_options.h"

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
      this->target = &desc;
      if (!desc.array_extents.empty()) {
         this->_state.array_extent = desc.array_extents[0];
      }
   }
   
   bitpacking::member_kind member_descriptor_view::kind() const {
      if (this->is_array())
         return bitpacking::member_kind::array;
      return this->target->kind;
   }
   gw::type::base member_descriptor_view::type() const {
      auto type = this->target->type;
      for(size_t i = 0; i < this->_state.array_rank; ++i)
         type = type.as_array().value_type();
      return type;
   }
   gw::type::base member_descriptor_view::innermost_value_type() const {
      return this->target->value_type;
   }
   
   const bitpacking::member_options::computed& member_descriptor_view::bitpacking_options() const {
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
      return this->_state.array_rank < ranks.size();
   }
   size_t member_descriptor_view::array_extent() const {
      return this->_state.array_extent;
   }
   
   member_descriptor_view& member_descriptor_view::descend_into_array() {
      auto& ranks = this->target->array_extents;
      assert(is_array());
      assert(this->_state.array_rank < ranks.size());
      
      ++this->_state.array_rank;
      if (this->_state.array_rank >= ranks.size()) {
         this->_state.array_extent = 1;
      } else {
         this->_state.array_extent = ranks[this->_state.array_rank];
      }
      
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
      gw::type::record type
   ) {
      this->type = type;
      type.for_each_field([this, &global](tree raw_decl) {
         auto decl = gw::decl::field::from_untyped(raw_decl);
         
         bitpacking::member_options::requested options;
         options.from_field(decl);
         if (options.omit_from_bitpacking) {
            return;
         }
         
         bitpacking::member_options::computed computed;
         computed.resolve(global, options, decl.value_type());
         
         auto& member = this->members.emplace_back();
         member.kind       = computed.kind;
         member.type       = decl.value_type();
         member.decl       = decl;
         member.value_type = member.type;
         if (member.type.is_array()) {
            auto array_type = member.type.as_array();
            auto value_type = array_type.value_type();
            do {
               if (computed.is_string() && value_type == global.types.string_char) {
                  break;
               }
               
               size_t extent;
               {
                  auto extent_opt = array_type.extent();
                  if (!extent_opt.has_value()) {
                     throw std::runtime_error(lu::strings::printf_string(
                        "struct data member %<%s::%s%> is a variable-length array; VLAs are not "
                        "supported",
                        this->type.pretty_print().c_str(),
                        member.decl.name().data()
                     ));
                  }
                  extent = *extent_opt;
               }
               member.array_extents.push_back(extent);
               
               if (!value_type.is_array()) {
                  break;
               }
               array_type = value_type.as_array();
               value_type = array_type.value_type();
            } while (true);
            member.value_type = value_type;
         }
         member.bitpacking_options = computed;
         
         switch (member.kind) {
            case bitpacking::member_kind::buffer:
               if (computed.buffer_options().bytecount > global.sectors.size_per) {
                  throw std::runtime_error(lu::strings::printf_string(
                     "buffer member %<%s::%s%> is too large (%u bytes) to fit in a sector, and splitting "
                     "buffers across sector boundaries is not implemented",
                     this->type.pretty_print().c_str(),
                     member.decl.name().data(),
                     computed.buffer_options().bytecount
                  ));
               }
               break;
            case bitpacking::member_kind::string:
               if (computed.string_options().length > global.sectors.size_per) {
                  throw std::runtime_error(lu::strings::printf_string(
                     "string member %<%s::%s%> is too large (%u bytes) to fit in a sector, and splitting "
                     "strings across sector boundaries is not implemented",
                     this->type.pretty_print().c_str(),
                     member.decl.name().data(),
                     computed.string_options().length
                  ));
               }
               break;
            default:
               break;
         }
      });
   }
   
   size_t struct_descriptor::size_in_bits() const {
      size_t bc = 0;
      for(auto& m : this->members)
         bc += m.size_in_bits();
      return bc;
   }
}