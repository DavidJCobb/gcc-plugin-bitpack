#include "gcc_wrappers/builtin_types.h"
#include <gcc-plugin.h>
#include <tree.h> // *_type_node
#include <c-family/c-common.h> // *_type_node

namespace gcc_wrappers {
   builtin_types::builtin_types() {
      this->basic_bool = type::base::from_untyped(boolean_type_node);
      this->basic_char = type::base::from_untyped(char_type_node);
      this->basic_int  = type::integral::from_untyped(integer_type_node);
      this->basic_void = type::base::from_untyped(void_type_node);
      this->int8  = type::integral::from_untyped(int8_type_node);
      this->int16 = type::integral::from_untyped(int16_type_node);
      this->int32 = type::integral::from_untyped(int32_type_node);
      this->int64 = type::integral::from_untyped(int64_type_node);
      this->uint8  = type::integral::from_untyped(uint8_type_node);
      this->uint16 = type::integral::from_untyped(c_uint16_type_node);
      this->uint32 = type::integral::from_untyped(c_uint32_type_node);
      this->uint64 = type::integral::from_untyped(c_uint64_type_node);
      
      this->intmax  = type::integral::from_untyped(intmax_type_node);
      this->uintmax = type::integral::from_untyped(uintmax_type_node);
      
      this->const_char_ptr = type::pointer::from_untyped(const_string_type_node);
      this->char_ptr       = type::pointer::from_untyped(string_type_node);
      //
      this->const_void_ptr = type::pointer::from_untyped(const_ptr_type_node);
      this->void_ptr       = type::pointer::from_untyped(ptr_type_node);
      
      this->intptr  = type::integral::from_untyped(intptr_type_node);
      this->uintptr = type::integral::from_untyped(uintptr_type_node);
      this->ptrdiff = type::integral::from_untyped(ptrdiff_type_node);
      this->size    = type::integral::from_untyped(size_type_node);
      this->ssize   = type::integral::from_untyped(signed_size_type_node);
      
      // If this assertion fails, then the singleton was created too early.
      gcc_assert(!this->uint8.empty());
   }
   
   type builtin_types::smallest_integral_for(size_t bitcount, bool is_signed) const {
      if (bitcount > 32)
         return is_signed ? int64 : uint64;
      if (bitcount > 16)
         return is_signed ? int32 : uint32;
      if (bitcount > 8)
         return is_signed ? int16 : uint16;
      return is_signed ? int8 : uint8;
   }
}