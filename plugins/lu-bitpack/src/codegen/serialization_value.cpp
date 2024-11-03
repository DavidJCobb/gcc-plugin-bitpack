#include "codegen/serialization_value.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace codegen {
   serialization_value serialization_value::access_member(const member_descriptor& desc) {
      assert(is_struct());
      
      serialization_value out;
      out.read = this->read.access_member(desc.decl.name());
      out.save = this->save.access_member(desc.decl.name());
      out.descriptor = member_descriptor_view(desc);
      
      return out;
   }
   serialization_value serialization_value::access_nth(value_pair n) {
      assert(is_array());
      assert(is_member());
      
      serialization_value out;
      out.read = this->read.access_nth(n.read);
      out.save = this->save.access_nth(n.save);
      out.descriptor = this->as_member().descended_into_array();
      
      return out;
   }
   serialization_value serialization_value::access_nth(size_t n) {
      const auto& ty = gw::builtin_types::get();
      return access_nth(value_pair{
         .read = gw::expr::integer_constant(ty.basic_int, n),
         .save = gw::expr::integer_constant(ty.basic_int, n),
      });
   }
   
   size_t serialization_value::bitcount() const {
      if (this->is_top_level_struct())
         return this->as_top_level_struct.size_in_bits();
      const auto& view = this->as_member();
      return view.type.size_in_bits();
   }
   bool serialization_value::is_array() const {
      if (this->is_top_level_struct())
         return false;
      const auto& view = this->as_member();
      return view.type.is_array();
   }
   bool serialization_value::is_struct() const {
      if (this->is_top_level_struct())
         return true;
      const auto& view = this->as_member();
      return view.type.is_struct();
   }
   
   size_t serialization_value::array_extent() const {
      assert(is_array());
      return this->as_member().array_extent();
   }
}