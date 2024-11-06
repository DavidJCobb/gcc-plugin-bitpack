#include "bitpacking/in_progress_options.h"
#include <stdexcept>
#include "lu/strings/printf_string.h"
#include "bitpacking/typed_data_options.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/type/base.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace bitpacking::in_progress_options {
   
   //
   // integral
   //
   
   void integral::coalesce(const integral& higher_prio) {
      if (auto& src = higher_prio.bitcount; src.has_value())
         this->bitcount = *src;
      if (auto& src = higher_prio.min; src.has_value())
         this->min = *src;
      if (auto& src = higher_prio.max; src.has_value())
         this->max = *src;
   }
   
   typed_data_options::integral integral::bake(gw::decl::field decl, gw::type::base type) const {
      typed_data_options::integral dst;
      
      //
      // Special-case integral types:
      //
      
      if (type.is_pointer()) {
         this->result.kind = member_kind::pointer;
         if (this->min.has_value() || this->max.has_value()) {
            throw std::runtime_error("specifying a minimum or maximum integer value for pointers is invalid");
         }
         dst.bitcount = type.size_in_bits();
         dst.min      = 0;
         dst.max      = std::numeric_limits<size_t>::max();
         if (this->bitcount.has_value() && *this->bitcount != dst.bitcount) {
            throw std::runtime_error("overriding the bitcount for pointers is invalid");
         }
         return dst;
      }
      
      if (
         type.is_boolean() &&
         this->bitcount.value_or(1) == 1 &&
         this->min.value_or(0) == 0 &&
         this->max.value_or(1) == 1
      ) {
         this->result.kind = member_kind::boolean;
         dst.bitcount = 1;
         dst.min      = 0;
         dst.max      = 1;
         return dst;
      }
      
      //
      // General-purpose integers and enums:
      //
      
      this->result.kind = member_kind::integer;
      if (this->bitcount.has_value()) {
         dst.bitcount = *this->bitcount;
         if (this->min.has_value())
            dst.min = *this->min;
         return dst;
      }
      intmax_t min = 0;
      intmax_t max = 0;
      if (type.is_enum() || type.is_integer()) {
         auto integral_type = type.as_integral();
         min = this->min.value_or(integral_type.minimum_value());
         if (this->max.has_value()) {
            max = *this->max;
         } else {
            max = integral_type.maximum_value();
            if (this->field.is_bitfield()) {
               auto bitfield_width = this->field.size_in_bits();
               assert(bitfield_width > 0);
               if (bitfield_width >= sizeof(intmax_t) * 8) {
                  throw std::runtime_error(lu::strings::printf_string("this member is a bitfield, and its width in bits (%zu) is too large for this plug-in to handle internally", bitfield_width));
                  max = min;
               } else {
                  max = 1 << (bitfield_width - 1);
               }
            }
         }
         if (min > max) {
            throw std::runtime_error(lu::strings::printf_string("the member's minimum value ("PRIdMAX") is greater than its maximum value ("PRIdMAX")", min, max));
         }
         dst.min      = min;
         dst.bitcount = std::bit_width((uintmax_t)(max - min));
      } else {
         dst.min      = 0;
         dst.max      = std::numeric_limits<intmax_t>::max();
         dst.bitcount = type.size_in_bits();
      }
      return dst;
   }
   
   //
   // string
   //
   
   void string::coalesce(const string& higher_prio) {
      if (auto& src = higher_prio.length; src.has_value())
         this->length = *src;
      if (auto& src = higher_prio.with_terminator; src.has_value())
         this->with_terminator = *src;
   }
   
   typed_data_options::string string::bake(
      const bitpacking::global_options::computed& global,
      gw::decl::field decl,
      gw::type::base type
   ) const {
      if (!global.type_is_string_or_array_thereof(type)) {
         throw std::runtime_error("string bitpacking options applied to a data member that is not a string or array thereof");
      }
      
      typed_data_options::string dst;
      
      if (src.with_terminator.has_value())
         dst.with_terminator = *src.with_terminator;
      
      //
      // The default length of a string is determined based on its type.
      //
      auto opt = global.string_extent_for_type(member_type);
      if (!opt.has_value()) {
         std::runtime_error("string bitpacking options applied to a variable-length array; VLAs are not supported");
      }
      //
      // Catch zero-length defaults.
      //
      dst.length = *opt;
      if (dst.length == 0) {
         std::runtime_error("string bitpacking options applied to a zero-length string");
      }
      if (dst.with_terminator) {
         --dst.length;
         if (dst.length == 0)
            std::runtime_error("string bitpacking options applied to a zero-length string (array of one character, to be spent on a null terminator)");
      }
      //
      // Pull the length from the coalesced options, if they supply one.
      //
      if (src.length.has_value()) {
         auto len = *src.length;
         if (len > dst.length) {
            std::runtime_error(lu::strings::printf_string(
               "the specified length (%u) is too large to fit in the target data member (array extent %zu)",
               len,
               dst.length
            ));
         }
         dst.length = len;
      }
      
      return dst;
   }
}