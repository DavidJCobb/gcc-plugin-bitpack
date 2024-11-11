#pragma once
#include <vector>
#include "bitpacking/data_options/computed.h"
#include "bitpacking/member_kind.h"
#include "codegen/descriptors/base_descriptor.h"
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/type/base.h"

namespace codegen {
   using data_options = bitpacking::data_options::computed;
   
   // FIELD_DECL or VAR_DECL
   class data_descriptor : public base_descriptor {
      protected:
         void _fail_if_indivisible_and_wont_fit() const;
      
      public:
         data_descriptor(gcc_wrappers::decl::field,    const data_options&);
         data_descriptor(gcc_wrappers::decl::variable, const data_options&);
      
         bitpacking::member_kind  kind = bitpacking::member_kind::none;
         gcc_wrappers::decl::base decl;
         
         size_t innermost_size_in_bits() const;
   };
}