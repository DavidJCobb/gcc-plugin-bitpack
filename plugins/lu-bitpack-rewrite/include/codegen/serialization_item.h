#pragma once
#include <memory> // GCC headers fuck up string-related STL identifiers that <memory> depends on
#include <string>
#include <vector>
#include "gcc_wrappers/type/base.h"

namespace bitpacking {
   namespace data_options {
      class computed;
   }
}
namespace codegen {
   class decl_descriptor;
}

namespace codegen {
   class serialization_item {
      public:
         // Where `count > 1`, this serialization item represents (a value somewhere within) 
         // a slice of an array, for the purposes of code generation. The segment itself may 
         // or may not be an array.
         //
         // For example, given `int foo[7][4]`, the segment `foo[2:6]` is an array, but the 
         // segment `foo[1][2:4]` is not.
         //
         struct array_access_info {
            size_t start = 0;
            size_t count = 1;
         };
         
         struct segment {
            public:
               const decl_descriptor* desc = nullptr;
               std::vector<array_access_info> array_accesses;
            
            public:
               const bitpacking::data_options::computed& options() const noexcept;
               
               size_t size_in_bits() const noexcept;
               
               bool is_array() const;
               size_t array_extent() const;
         };
         
      public:
         struct {
            bool defaulted : 1 = false;
            bool omitted   : 1 = false;
         } flags;
         std::vector<segment> segments;
         
      public:
         const decl_descriptor& descriptor() const noexcept;
         const bitpacking::data_options::computed& options() const noexcept;
         
         size_t size_in_bits() const;
         
         bool can_expand() const;
         
         std::vector<serialization_item> expanded() const;
         
         std::string to_string() const;
   };
}