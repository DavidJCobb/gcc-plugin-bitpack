#pragma once
#include <vector>
#include "bitpacking/data_options/computed.h"
#include "gcc_wrappers/type/base.h"

namespace codegen {
   using data_options = bitpacking::data_options::computed;
   
   class base_descriptor {
      protected:
         void _init_type();
      
      public:
         // for TYPEs:
         base_descriptor(gcc_wrappers::type::base type);
         
         // for DECLs:
         base_descriptor(gcc_wrappers::type::base type, const data_options&);
      
      public:
         gcc_wrappers::type::base type;
         gcc_wrappers::type::base innermost_type;
         data_options             options;
         //
         struct {
            std::vector<size_t> extents;
            bool                is_variable_length = false;
         } array;
   };
}