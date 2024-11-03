#pragma once
#include <memory>
#include <vector>
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/type.h"
#include "bitpacking/member_kind.h"
#include "bitpacking/member_options.h"

namespace bitpacking {
   namespace global_options {
      class computed;
   }
}

namespace codegen {
   class member_descriptor {
      public:
         bitpacking::member_kind   kind; // for arrays, is the innermost value type's kind
         gcc_wrappers::type        type;
         gcc_wrappers::type        value_type; // if `type` is `int(&)[3][2]`, this is `int`
         gcc_wrappers::decl::field decl;
         
         // for arrays only. given `int foo[4][3][2]`, this would be `{ 4, 3, 2 }`.
         std::vector<size_t> array_extents;
         
         bitpacking::member_options::computed bitpacking_options;
         
         size_t size_in_bits() const; // given int foo[4][3][2], this is the bitcount of any foo[x][y][z]
   };
   
   class struct_descriptor {
      public:
         gcc_wrappers::type type;
         std::vector<member_descriptor> members;
         
         struct_descriptor(const bitpacking::global_options::computed&, gcc_wrappers::type);
         size_t size_in_bits() const;
   };
   
   class member_descriptor_view {
      protected:
         const member_descriptor* target = nullptr;
         struct {
            // given int foo[4][3][2], rank 0 has extent 4, rank 1 has extent 3, 
            // rank 2 has extent 2, and "rank 3" indicates that we're dealing with 
            // individual ints.
            size_t array_rank   = 0;
            
            size_t array_extent = 1;
         } _state;
         
      public:
         member_descriptor_view(const member_descriptor&);
      
         bitpacking::member_kind kind() const;
         gcc_wrappers::type type() const;
         gcc_wrappers::type innermost_value_type() const;
         
         const bitpacking::member_options::computed& bitpacking_options() const;
      
         size_t size_in_bits() const; // given int foo[4][3][2], this is the bitcount of foo as a whole
         size_t element_size_in_bits() const; // given int foo[4][3][2], this is the bitcount of foo[x][y][z]
         
         bool is_array() const;
         size_t array_extent() const; // returns 1 for non-arrays
         
         member_descriptor_view& descend_into_array();
         member_descriptor_view descended_into_array() const;
   };
}