#include "bitpacking/data_options/computed.h"
#include "bitpacking/data_options/requested.h"
#include "gcc_wrappers/type/array.h"
#include "debugprint.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace bitpacking::data_options {
   void computed::_load_impl(const requested& src, gcc_wrappers::type::base type, std::optional<size_t> bitfield_width) {
      if (src.omit)
         this->omit_from_bitpacking = true;
      
      if (src.failed)
         return;
      
      if (src.default_value_node != NULL_TREE)
         this->default_value_node = src.default_value_node;
      
      if (src.has_attr_nonstring)
         this->has_attr_nonstring = true;
      
      if (src.union_member_id.has_value())
         this->union_member_id = *src.union_member_id;
      
      gw::type::base value_type = type;
      while (value_type.is_array())
         value_type = value_type.as_array().value_type();
      
      if (std::holds_alternative<requested_x_options::buffer>(src.x_options)) {
         this->kind = member_kind::buffer;
         auto& dst = this->data.emplace<computed_x_options::buffer>();
         dst.bytecount = value_type.size_in_bytes();
      } else if (auto* casted = std::get_if<requested_x_options::integral>(&src.x_options)) {
         this->data = casted->bake(value_type, this->kind, bitfield_width);
         assert(this->kind != member_kind::none);
      } else if (auto* casted = std::get_if<requested_x_options::string>(&src.x_options)) {
         this->kind = member_kind::string;
         this->data = casted->bake(type.as_array());
      } else if (auto* casted = std::get_if<requested_x_options::transforms>(&src.x_options)) {
         this->kind = member_kind::transformed;
         this->data = *casted;
      }  else if (auto* casted = std::get_if<requested_x_options::tagged_union>(&src.x_options)) {
         this->kind = member_kind::union_external_tag;
         if (casted->is_internal)
            this->kind = member_kind::union_internal_tag;
         this->data = computed_x_options::tagged_union{
            .tag_identifier = casted->tag_identifier,
            .is_internal    = casted->is_internal,
         };
      } else {
         if (value_type.is_pointer() || value_type.is_boolean() || value_type.is_integer() || value_type.is_enum()) {
            this->data = requested_x_options::integral{}.bake(value_type, this->kind, bitfield_width);
            assert(this->kind != member_kind::none);
         } else if (value_type.is_floating_point()) {
            //
            // Nothing special we can do for floats, so just pack them as an 
            // opaque buffer.
            //
            this->kind = member_kind::buffer;
            auto& dst = this->data.emplace<computed_x_options::buffer>();
            dst.bytecount = value_type.size_in_bytes();
         } else if (value_type.is_record()) {
            this->kind = member_kind::structure;
         } else if (value_type.is_union()) {
            assert(false && "not yet implemented");
         } else {
            if (!src.omit) {
               if (value_type.is_union()) {
                  assert(false && "not yet implemented");
               } else {
                  assert(false && "no bitpacking options + unhandled member type");
               }
            }
         }
      }
   }
   
   bool computed::load(gw::decl::field decl) {
      requested src;
      src.load(decl);
      
      std::optional<size_t> bitfield_width;
      if (decl.is_bitfield()) {
         bitfield_width = decl.size_in_bits();
      }
      this->_load_impl(src, decl.value_type(), bitfield_width);
      
      return !src.failed;
   }
   bool computed::load(gw::decl::param decl) {
      // We should only end up querying options for a parameter when we're 
      // dealing with the struct pointer argument for a whole-struct function; 
      // those functions are generated, so the parameters should never have 
      // any arguments. Just act on the type.
      auto type = decl.value_type();
      
      requested src;
      src.load(type);
      
      this->_load_impl(src, type, {});
      
      return !src.failed;
   }
   bool computed::load(gw::decl::variable decl) {
      requested src;
      src.load(decl);
      this->_load_impl(src, decl.value_type(), {});
      return !src.failed;
   }
   bool computed::load(gw::type::base type) {
      requested src;
      src.load(type);
      
      this->_load_impl(src, type, {});
      
      return !src.failed;
   }
}