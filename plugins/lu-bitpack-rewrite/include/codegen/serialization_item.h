#pragma once
#include <memory> // GCC headers fuck up string-related STL identifiers that <memory> depends on
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "gcc_wrappers/type/base.h"
#include "codegen/serialization_items/segment.h"
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
         using basic_segment   = serialization_items::basic_segment;
         using padding_segment = serialization_items::padding_segment;
         using condition_type  = serialization_items::condition_type;
         using segment         = serialization_items::segment;
         
      public:
         bool is_defaulted : 1 = false;
         bool is_omitted   : 1 = false;
         //
         std::vector<segment> segments;
         
      public:
         void append_segment(const decl_descriptor&);
      
         const decl_descriptor& descriptor() const noexcept;
         const bitpacking::data_options::computed& options() const noexcept;
         
         size_t size_in_bits() const;
         
         bool can_expand() const;
         
         bool is_padding() const;
         bool is_union() const;
      
      protected:
         std::vector<serialization_item> _expand_record() const;
         std::vector<serialization_item> _expand_externally_tagged_union() const;
         std::vector<serialization_item> _expand_internally_tagged_union() const;
      
      public:
         std::vector<serialization_item> expanded() const;
         
         std::string to_string() const;
   };
}