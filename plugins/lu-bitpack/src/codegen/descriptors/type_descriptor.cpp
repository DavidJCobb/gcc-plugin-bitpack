#include "codegen/descriptors/type_descriptor.h"
#include "gcc_wrappers/type/container.h"

namespace codegen {
   type_descriptor::type_descriptor(gcc_wrappers::type::base type)
   :
      base_descriptor(type)
   {
      if (this->type.is_record() || this->type.is_union()) {
         auto cont = this->type.as<gw::type::container>();
         assert(!cont.empty());
         cont.for_each_referenceable_field([this](tree raw) {
            auto decl = gw::decl::field::from_untyped(raw);
            
            data_options computed;
            if (!computed.load(decl)) {
               auto message = lu::strings::printf_string(
                  "unable to continue: a problem occurred while processing bitpacking options for struct data member %<%s::%s%>",
                  this->type.pretty_print().c_str(),
                  decl.name().data()
               );
               throw std::runtime_error(message);
            }
            if (computed.omit_from_bitpacking) {
               return;
            }
            
            this->members.emplace_back(decl, computed);
         });
      }
   }
   
   bool type_descriptor::is_omitted_from_bitpacking() const {
      return this->options.omit_from_bitpacking;
   }
   
   size_t type_descriptor::innermost_size_in_bits() const {
      if (is_omitted_from_bitpacking())
         return 0;
      
      if (is_struct()) {
         size_t bc = 0;
         for(auto& m : this->members) {
            size_t m_bc = m.innermost_size_in_bits();
            for(auto extent : m.array_extents)
               m_bc *= extent;
            
            bc += m_bc;
         }
         return bc;
      }
      if (is_union()) {
         size_t bc = 0;
         for(auto& m : this->members) {
            size_t m_bc = m.innermost_size_in_bits();
            for(auto extent : m.array_extents)
               m_bc *= extent;
            
            if (bc < m_bc)
               bc = m_bc;
         }
         return bc;
      }
      
      assert(this->members.empty());
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
      return this->type.size_in_bits();
   }
   size_t type_descriptor::size_in_bits() const {
      auto bitcount = this->innermost_size_in_bits();
      if (bitcount == 0)
         return bitcount;
      
      for(auto extent : this->array.extents) {
         if (extent == 0)
            throw std::runtime_error("failed to compute a type's serialized size in bits, possibly because it was a variable-length array");
         bitcount *= extent;
      }
      
      return bitcount;
   }
   
   bool type_descriptor::is_struct() const {
      return this->innermost_type.is_record();
   }
   bool type_descriptor::is_union() const {
      return this->innermost_type.is_union();
   }
   
}