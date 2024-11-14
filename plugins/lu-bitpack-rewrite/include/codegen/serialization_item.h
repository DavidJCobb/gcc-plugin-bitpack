#pragma once
#include <memory> // GCC headers fuck up string-related STL identifiers that <memory> depends on
#include <string>
#include <vector>
#include "gcc_wrappers/type/base.h"
#include "codegen/array_access_info.h"

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
            bool padding   : 1 = false;
         } flags;
         std::vector<condition> conditions;
         std::vector<segment>   segments;
         
         // Only used, and the only thing that *is* used, if `padding` flag is set.
         // Measured in bits.
         size_t padding_size = 0;
         
      public:
         const decl_descriptor& descriptor() const noexcept;
         const bitpacking::data_options::computed& options() const noexcept;
         
         size_t size_in_bits() const;
         
         bool can_expand() const;
         
         bool is_union() const;
         
         std::vector<serialization_item> expanded() const;
         
         std::string to_string() const;
         
         // True if this item's condition list equals the other item's condition list with 
         // more conditions added.
         bool conditions_are_a_narrowing_of(const serialization_item&);
   };
}