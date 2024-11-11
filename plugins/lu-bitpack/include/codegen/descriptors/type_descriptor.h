#pragma once
#include <vector>
#include "bitpacking/data_options/computed.h"
#include "codegen/descriptors/data_descriptor.h"
#include "gcc_wrappers/type/base.h"

namespace codegen {
   using data_options = bitpacking::data_options::computed;
   
   // TYPE_DECL or non-array TYPE
   class type_descriptor {
      public:
         type_descriptor(gcc_wrappers::type::base);
      
         gcc_wrappers::type::base     type;
         gcc_wrappers::type::base     innermost_type;
         std::vector<data_descriptor> members;
         
         data_options options;
         
         bool is_omitted_from_bitpacking() const;
         
         size_t innermost_size_in_bits() const;
         size_t size_in_bits() const;
         
         bool is_struct() const;
         bool is_union() const;
   };
}