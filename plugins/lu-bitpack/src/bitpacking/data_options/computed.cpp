#include "bitpacking/data_options/computed.h"
#include "bitpacking/data_options/requested.h"
#include "gcc_wrappers/type/array.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace bitpacking::data_options {
   bool computed::load(gw::decl::field decl) {
      requested src;
      src.load(decl);
      
      if (src.omit)
         this->omit_from_bitpacking = true;
      
      if (src.failed) {
         return false;
      }
      
      gw::type::base value_type = decl.value_type();
      while (value_type.is_array())
         value_type = value_type.as_array().value_type();
      
      if (std::holds_alternative<requested_x_options::buffer>(src.x_options)) {
         this->kind = member_kind::buffer;
         auto& dst = this->data.emplace<computed_x_options::buffer>();
         dst.bytecount = value_type.size_in_bytes();
      } else if (auto* casted = std::get_if<requested_x_options::integral>(&src.x_options)) {
         this->data = casted->bake(decl, this->kind);
         assert(this->kind != member_kind::none);
      } else if (auto* casted = std::get_if<requested_x_options::string>(&src.x_options)) {
         this->kind = member_kind::string;
         this->data = casted->bake(decl);
      } else if (auto* casted = std::get_if<requested_x_options::transforms>(&src.x_options)) {
         this->kind = member_kind::transformed;
         this->data = *casted;
      } else {
         if (value_type.is_pointer() || value_type.is_boolean() || value_type.is_integer() || value_type.is_enum()) {
            this->data = requested_x_options::integral{}.bake(decl, this->kind);
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
      
      return true;
   }
}