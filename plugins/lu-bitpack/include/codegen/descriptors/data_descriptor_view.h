#pragma once
#include "bitpacking/data_options/computed.h"
#include "codegen/descriptors/data_descriptor.h"
#include "gcc_wrappers/type/base.h"

namespace codegen {
   using data_options = bitpacking::data_options::computed;
   
   class data_descriptor;
   
   class data_descriptor_view {
      protected:
         const data_descriptor* _target = nullptr;
         
         // given int foo[4][3][2], rank 0 has extent 4, rank 1 has extent 3, 
         // rank 2 has extent 2, and "rank 3" indicates that we're dealing with 
         // individual ints.
         size_t _array_rank = 0;
         
      public:
         data_descriptor_view(const data_descriptor&);
         
         constexpr const data_descriptor_view& innermost_member() const noexcept {
            return *this->_target;
         }
         
         bitpacking::member_kind kind() const;
         bitpacking::member_kind innermost_kind() const;
         
         gcc_wrappers::type::base type() const;
         gcc_wrappers::type::base innermost_type() const;
         
         data_options options() const;
         
         bool   is_array() const;
         size_t array_extent() const; // returns 1 for non-arrays
         
         data_descriptor_view& descend_into_array();
         data_descriptor_view  descended_into_array() const;
         
         size_t size_in_bits() const; // given int foo[4][3][2], this is the bitcount of foo as a whole
         size_t element_size_in_bits() const; // given int foo[4][3][2], this is the bitcount of foo[x][y][z]
         
         // given int foo[4][3][2], this is 4 * 3 * 2
         size_t count_of_innermost() const;
   };
}