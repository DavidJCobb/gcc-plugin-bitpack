#include "xmlgen/serialization_value.h"
#include "lu/strings/printf_string.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace xmlgen {
   serialization_value serialization_value::access_member(const codegen::member_descriptor& desc) const {
      assert(is_struct());
      
      auto name = desc.decl.name();
      
      serialization_value out;
      out.descriptor = codegen::member_descriptor_view(desc);
      out.path = this->path;
      out.path += '.';
      out.path += name;
      
      return out;
   }
   serialization_value serialization_value::access_nth(size_t start, size_t count) const {
      assert(is_array());
      assert(is_member());
      
      serialization_value out;
      out.descriptor = this->as_member().descended_into_array();
      out.path = this->path;
      if (count == 1) {
         out.path += lu::strings::printf_string("[%u]", start);
      } else {
         out.path += lu::strings::printf_string("[%u:%u]", start, start + count);
      }
      
      return out;
   }
   serialization_value serialization_value::access_nth(size_t n) const {
      return access_nth(n, 1);
   }
   
   size_t serialization_value::bitcount() const {
      if (this->is_top_level_struct())
         return this->as_top_level_struct().size_in_bits();
      const auto& view = this->as_member();
      return view.size_in_bits();
   }
   bool serialization_value::is_array() const {
      if (this->is_top_level_struct())
         return false;
      const auto& view = this->as_member();
      return view.is_array();
   }
   bool serialization_value::is_struct() const {
      if (this->is_top_level_struct())
         return true;
      const auto& view = this->as_member();
      return !view.is_array() && view.type().is_record();
   }
   
   size_t serialization_value::array_extent() const {
      assert(is_array());
      return this->as_member().array_extent();
   }
}