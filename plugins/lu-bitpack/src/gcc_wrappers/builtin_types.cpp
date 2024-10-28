#include "gcc_wrappers/builtin_types.h"
#include <gcc-plugin.h>
#include <tree.h> // *_type_node
#include <c-family/c-common.h> // *_type_node

namespace gcc_wrappers {
   builtin_types::builtin_types() {
      this->basic_bool = type::from_untyped(boolean_type_node);
      this->basic_char = type::from_untyped(char_type_node);
      this->basic_int  = type::from_untyped(integer_type_node);
      this->basic_void = type::from_untyped(void_type_node);
      this->int8  = type::from_untyped(int8_type_node);
      this->int16 = type::from_untyped(int16_type_node);
      this->int32 = type::from_untyped(int32_type_node);
      this->int64 = type::from_untyped(int64_type_node);
      this->uint8  = type::from_untyped(uint8_type_node);
      this->uint16 = type::from_untyped(c_uint16_type_node);
      this->uint32 = type::from_untyped(c_uint32_type_node);
      this->uint64 = type::from_untyped(c_uint64_type_node);
      
      this->intmax  = type::from_untyped(intmax_type_node);
      this->uintmax = type::from_untyped(uintmax_type_node);
      
      this->const_char_ptr = type::from_untyped(const_string_type_node);
      this->char_ptr       = type::from_untyped(string_type_node);
      //
      this->const_void_ptr = type::from_untyped(const_ptr_type_node);
      this->void_ptr       = type::from_untyped(ptr_type_node);
      
      this->intptr  = type::from_untyped(intptr_type_node);
      this->uintptr = type::from_untyped(uintptr_type_node);
      this->ptrdiff = type::from_untyped(ptrdiff_type_node);
      this->size    = type::from_untyped(size_type_node);
      this->ssize   = type::from_untyped(signed_size_type_node);
   }
}