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
         // segment `foo[1][2:4]` is not. That said, the `size_in_bits` of `foo[1][2:4]` is 
         // equal to the `size_in_bits` of `foo[1][0]` times two.
         //
         struct array_access_info {
            bool operator==(const array_access_info&) const noexcept = default;
            
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
               
               // Intentionally does not include transformations.
               std::string to_string() const;
               
            public:
               bool operator==(const segment&) const noexcept = default;
         };
         
         // For serializing tagged unions: each union member (and the data therein) will be 
         // conditional on the union tag value. Multiple AND-linked conditions may be present 
         // (when dealing with nested unions).
         //
         struct condition {
            bool operator==(const condition&) const noexcept = default;
            
            std::vector<segment> path;
            intmax_t value = 0;
            
            std::string to_string() const;
         };
         
      public:
         struct _ {
            bool operator==(const _&) const noexcept = default;
            
            bool defaulted : 1 = false;
            bool omitted   : 1 = false;
         } flags;
         std::vector<condition> conditions;
         std::vector<segment>   segments;
         
      public:
         const decl_descriptor& descriptor() const noexcept;
         const bitpacking::data_options::computed& options() const noexcept;
         
         size_t size_in_bits() const;
         
         bool can_expand() const;
         
         std::vector<serialization_item> expanded() const;
         
         std::string to_string() const;
   };
}