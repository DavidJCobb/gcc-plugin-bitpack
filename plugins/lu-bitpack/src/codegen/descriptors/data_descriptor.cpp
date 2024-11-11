#include "codegen/descriptors/data_descriptor.h"
#include <cassert>
#include <stdexcept>
#include "lu/strings/printf_string.h"
#include "gcc_wrappers/type/array.h"
#include "basic_global_state.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace codegen {
   void data_descriptor::_fail_if_indivisible_and_wont_fit() const {
      const auto& global = basic_global_state::get().global_options.computed;
      switch (member.kind) {
         case bitpacking::member_kind::buffer:
            if (computed.buffer_options().bytecount > global.sectors.size_per) {
               const auto fq = this->decl.fully_qualified_name();
               throw std::runtime_error(lu::strings::printf_string(
                  "opaque buffer %<%s%> is too large (%u bytes) to fit in a sector, and splitting opaque buffers across sector boundaries is not implemented",
                  fq.c_str(),
                  computed.buffer_options().bytecount
               ));
            }
            break;
         case bitpacking::member_kind::string:
            if (computed.string_options().length > global.sectors.size_per) {
               const auto fq = this->decl.fully_qualified_name();
               throw std::runtime_error(lu::strings::printf_string(
                  "string %<%s%> is too large (%u bytes) to fit in a sector, and splitting strings across sector boundaries is not implemented",
                  fq.c_str(),
                  computed.string_options().length
               ));
            }
            break;
         default:
            break;
      }
   }
   
   data_descriptor::data_descriptor(gw::decl::field decl, const data_options& computed)
   :
      base_descriptor(decl.value_type(), computed)
   {
      this->kind = computed.kind;
      this->decl = decl;
      
      if (this->array.is_variable_length) {
         auto fq = this->decl.fully_qualified_name();
         throw std::runtime_error(lu::strings::printf_string(
            "field %<%s%> is a variable-length array; VLAs are not supported",
            fq.c_str()
         ));
      }
      this->_fail_if_indivisible_and_wont_fit();
   }
   data_descriptor::data_descriptor(gw::decl::variable decl, const data_options& computed)
   :
      base_descriptor(decl.value_type(), computed)
   {
      this->kind = computed.kind;
      this->decl = decl;
      
      if (this->array.is_variable_length) {
         throw std::runtime_error(lu::strings::printf_string(
            "variable %<%s%> is a variable-length array; VLAs are not supported",
            this->decl.name().data()
         ));
      }
      this->_fail_if_indivisible_and_wont_fit();
   }
   
   size_t data_descriptor::innermost_size_in_bits() const {
      const auto& o = this->options;
      if (o.is_buffer()) {
         return o.buffer_options().bytecount * 8;
      } else if (o.is_integral()) {
         return o.integral_options().bitcount;
      } else if (o.is_string()) {
         return o.string_options().length * 8;
      } else if (o.is_transformed()) {
         static_assert(false, "TODO");
      }
      return this->innermost_type.size_in_bits();
   }
}