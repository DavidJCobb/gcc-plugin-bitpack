#include "bitpacking/structure.h"
#include <bit>

namespace codegen {
   structure_member::structure_member(gcc_wrappers::decl::field decl) : decl(decl) {
      this->type       = decl.value_type();
      this->value_type = this->type;
      while (this->value_type.is_array()) {
         this->value_type = this->value_type.array_value_type();
      }
      
      static_assert(false, "TODO: this->serialization_type");
   }
   
   void structure_member::recompute_options() {
      const auto& src = this->options.specified;
      auto&       dst = this->options.computed;
      switch (this->serialization_type) {
         case member_serialization_type::buffer:
            dst.buffer.bytecount = this->value_type.size_in_bytes();
            break;
         case member_serialization_type::boolean:
            [[fallthrough]];
         case member_serialization_type::integer:
            if (src.integral.bitcount.has_value()) {
               dst.integral.bitcount = *src.integral.bitcount;
               break;
            }
            if (!src.integral.min.has_value() && !src.integral.max.has_value()) {
               if (this->serialization_type == member_serialization_type::boolean) {
                  return 1;
               }
               return this->decl.size_in_bits();
            }
            {
               auto integral_info = this->type.get_integral_info();
               
               std::intmax_t  min;
               std::uintmax_t max;
               if (src.integral.min.has_value()) {
                  min = *src.integral.min;
               } else {
                  min = integral_info.min;
               }
               if (src.integral.max.has_value()) {
                  max = *src.integral.max;
               } else {
                  max = integral_info.max;
               }
               
               return std::bit_width((std::uintmax_t)(max - min));
            }
            break;
         case member_serialization_type::pointer:
            break;
         case member_serialization_type::string:
            assert(src.string.length.has_value());
            dst.string.length          = *src.string.length;
            dst.string.null_terminated = src.string.null_terminated;
            break;
         case member_serialization_type::substructure:
            static_assert(false, "TODO");
            break;
      }
   }
   
   size_t structure_member::compute_single_element_packed_bitcount() const {
      const auto& o = this->options.computed;
      switch (this->serialization_type) {
         case member_serialization_type::boolean:
         case member_serialization_type::integer:
            return o.integral.bitcount;
         case member_serialization_type::buffer:
            return o.buffer.bytecount;
         case member_serialization_type::pointer:
            return this->value_type.size_in_bits();
         case member_serialization_type::string:
            {
               size_t bytes = o.string.length;
               if (o.string.null_terminated) {
                  ++bytes;
               }
               return bytes * this->value_type.size_in_bits();
            }
            break;
         case member_serialization_type::substructure:
            return o.substructure.bitcount;
      }
      return 0;
   }
   size_t structure_member::compute_single_element_unpacked_bytecount() const {
      const auto& o = this->options.computed;
      switch (this->serialization_type) {
         case member_serialization_type::boolean:
         case member_serialization_type::integer:
            {
               auto bc = this->decl.size_in_bits();
               if (bc < 8)
                  bc = 8;
               return (bc / 8) + ((bc % 8) ? 1 : 0);
            }
            break;
         case member_serialization_type::buffer:
            return this->value_type.size_in_bytes();
         case member_serialization_type::pointer:
            return this->value_type.size_in_bytes();
         case member_serialization_type::string:
            {
               size_t bytes = o.string.length;
               if (o.string.null_terminated) {
                  ++bytes;
               }
               return bytes * sizeof(char);
            }
            break;
         case member_serialization_type::substructure:
            static_assert(false, "TODO");
            break;
      }
      return 0;
   }
   
   size_t structure_member::compute_total_packed_bitcount() const {
      size_t count = this->compute_single_element_packed_bitcount();
      {
         auto type = this->type;
         while (type.is_array()) {
            auto extent = type.array_extent();
            assert(extent.has_value() && "cannot bitpack VLAs");
            count *= *extent;
            
            type = type.array_value_type();
         }
      }
      return count;
   }
   size_t structure_member::compute_total_unpacked_bitcount() const {
      size_t count = this->compute_single_element_unpacked_bytecount();
      {
         auto type = this->type;
         while (type.is_array()) {
            auto extent = type.array_extent();
            assert(extent.has_value() && "cannot bitpack VLAs");
            count *= *extent;
            
            type = type.array_value_type();
         }
      }
      return count;
   }
   
   //
   // structure
   //
   
   structure::structure(gcc_wrappers::type t) : type(t) {
      t.for_each_field([this](tree raw_decl) {
         auto decl = gcc_wrappers::decl::field::from_untyped(raw_decl);
         auto attr = decl.attributes();
         bool skip = false;
         attr.for_each_kv_pair([&skip](tree k, tree v) -> bool {
            if (TREE_CODE(k) != IDENTIFIER_NODE)
               return true;
            std::string_view key = IDENTIFIER_POINTER(k);
            if (key == "lu_bitpack_omit") {
               skip = true;
               return false;
            }
            return true;
         };
         if (skip)
            continue;
         
         auto& member = this->members.emplace_back(decl);
         member.recompute_options();
      });
   }
   
   size_t structure::compute_packed_bitcount() const {
      size_t c = 0;
      for (const auto& m : this->members)
         c += m.compute_total_packed_bitcount();
      return c;
   }
   size_t structure::compute_unpacked_bytecount() const {
      size_t c = 0;
      for (const auto& m : this->members)
         c += m.compute_total_unpacked_bitcount();
      return c;
   }
}