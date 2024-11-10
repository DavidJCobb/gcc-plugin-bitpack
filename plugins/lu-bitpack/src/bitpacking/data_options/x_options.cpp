#include "bitpacking/data_options/x_options.h"
#include <bit>
#include <stdexcept>
#include "lu/strings/printf_string.h"
#include "bitpacking/global_options.h"
#include "bitpacking/member_kind.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/array.h"
#include "basic_global_state.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace bitpacking::data_options::requested_x_options {
   
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
   
   computed_x_options::integral integral::bake(
      gw::decl::field decl,
      member_kind&    out_kind
   ) const {
      const auto& global = basic_global_state::get().global_options.computed;
      
      auto type = decl.value_type();
      while (type.is_array())
         type = type.as_array().value_type();
      
      computed_x_options::integral dst;
      
      //
      // Special-case integral types:
      //
      
      if (type.is_pointer()) {
         out_kind = member_kind::pointer;
         dst.bitcount = type.size_in_bits();
         dst.min      = 0;
         dst.max      = std::numeric_limits<size_t>::max();
         return dst;
      }
      
      if (
         global.type_is_boolean(type) &&
         this->bitcount.value_or(1) == 1 &&
         this->min.value_or(0) == 0 &&
         this->max.value_or(1) == 1
      ) {
         out_kind     = member_kind::boolean;
         dst.bitcount = 1;
         dst.min      = 0;
         dst.max      = 1;
         return dst;
      }
      
      //
      // General-purpose integers and enums:
      //
      
      out_kind = member_kind::integer;
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
            if (decl.is_bitfield()) {
               auto bitfield_width = decl.size_in_bits();
               assert(bitfield_width > 0);
               if (bitfield_width >= sizeof(intmax_t) * 8) {
                  throw std::runtime_error(lu::strings::printf_string("this member is a bitfield, and its width in bits (%zu) is too large for this plug-in to handle internally", bitfield_width));
                  max = min;
               } else {
                  max = 1 << (bitfield_width - 1);
               }
            }
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
   
   computed_x_options::string string::bake(gw::decl::field decl) const {
      computed_x_options::string dst;
      
      if (this->nonstring.has_value())
         dst.nonstring = *this->nonstring;
      
      gw::type::array array_type = decl.value_type().as_array();
      gw::type::base  value_type = array_type.value_type();
      while (value_type.is_array()) {
         array_type = value_type.as_array();
         value_type = array_type.value_type();
      }
      dst.length = *array_type.extent();
      assert(dst.length > 0); // should've been validated by the string attribute handler
      if (!dst.nonstring) {
         --dst.length;
      }
      
      return dst;
   }
}