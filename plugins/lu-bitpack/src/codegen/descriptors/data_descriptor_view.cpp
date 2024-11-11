#include "codegen/descriptors/base_descriptor.h"
#include <cassert>
#include <stdexcept>
#include "lu/strings/printf_string.h"
#include "gcc_wrappers/type/array.h"
#include "basic_global_state.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace codegen {
   data_descriptor_view::data_descriptor_view(const data_descriptor& target) {
      this->_target = &target;
   }
   
   bitpacking::member_kind data_descriptor_view::kind() const {
      if (this->is_array())
         return bitpacking::member_kind::array;
      return this->innermost_kind();
   }
   bitpacking::member_kind data_descriptor_view::innermost_kind() const {
      return this->_target->kind;
   }
   
   gw::type::base data_descriptor_view::type() const {
      auto type = this->_target->type;
      for(size_t i = 0; i < this->_array_rank; ++i)
         type = type.as_array().value_type();
      return type;
   }
   gw::type::base data_descriptor_view::innermost_type() const {
      return this->_target->innermost_type;
   }
   
   data_options data_descriptor_view::options() const {
      return this->_target->bitpacking_options;
   }
   
   bool data_descriptor_view::is_array() const {
      auto& ranks = this->_target->array.extents;
      if (ranks.empty())
         return false;
      return this->_array_rank < ranks.size();
   }
   size_t data_descriptor_view::array_extent() const {
      auto& ranks = this->_target.array.extents;
      if (this->_array_rank >= ranks.size())
         return 1;
      return ranks[this->_array_rank];
   }
   
   data_descriptor_view& data_descriptor_view::descend_into_array() {
      auto& ranks = this->_target->array.extents;
      assert(is_array());
      assert(this->_array_rank < ranks.size());
      ++this->_array_rank;
      return *this;
   }
   data_descriptor_view data_descriptor_view::descended_into_array() const {
      auto it = *this;
      return it.descend_into_array();
   }
   
   size_t data_descriptor_view::size_in_bits() const {
      return this->element_size_in_bits() * this->array_extent();
   }
   size_t data_descriptor_view::element_size_in_bits() const {
      size_t bc    = this->_target->size_in_bits();
      auto&  ranks = this->_target->array.extents;
      for(size_t i = this->_state.array_rank + 1; i < ranks.size(); ++i) {
         bc *= ranks[i];
      }
      return bc;
   }
   
   size_t data_descriptor_view::count_of_innermost() const {
      if (!is_array())
         return 1;
      size_t count = 1;
      auto&  ranks = this->_target->array.extents;
      for(size_t i = this->_state.array_rank; i < ranks.size(); ++i) {
         count *= ranks[i];
      }
      return count;
   }
}