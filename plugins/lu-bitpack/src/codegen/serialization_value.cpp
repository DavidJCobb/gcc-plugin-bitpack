#include "codegen/serialization_value.h"

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
}