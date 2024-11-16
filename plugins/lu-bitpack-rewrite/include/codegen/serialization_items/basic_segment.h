#pragma once
#include <string>
#include <vector>
#include "codegen/array_access_info.h"

namespace bitpacking::data_options {
   class computed;
}
namespace codegen {
   class decl_descriptor;
}

namespace codegen::serialization_items {
   class basic_segment {
      public:
         bool operator==(const basic_segment&) const noexcept = default;
         
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
   };
}