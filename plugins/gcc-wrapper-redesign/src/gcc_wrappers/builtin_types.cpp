#include "gcc_wrappers/builtin_types.h"
#include <cassert>
#include <gcc-plugin.h>
#include <tree.h> // *_type_node
#include <c-family/c-common.h> // *_type_node

namespace gcc_wrappers {
   builtin_types::builtin_types()
   :
      basic_bool(
         assert(boolean_type_node != NULL_TREE && "You must be careful not to create this singleton too early, i.e. before GCC has initialized the built-in C types."),
         type::base::wrap(boolean_type_node)
      ),
      basic_char(type::base::wrap(char_type_node)),
      basic_int (type::integral::wrap(integer_type_node)),
      basic_void(type::base::from_untyped(void_type_node)),
      //
      int8  (type::integral::from_untyped(int8_type_node)),
      int16 (type::integral::from_untyped(int16_type_node)),
      int32 (type::integral::from_untyped(int32_type_node)),
      int64 (type::integral::from_untyped(int64_type_node)),
      uint8 (type::integral::from_untyped(uint8_type_node)),
      uint16(type::integral::from_untyped(c_uint16_type_node)),
      uint32(type::integral::from_untyped(c_uint32_type_node)),
      uint64(type::integral::from_untyped(c_uint64_type_node)),
      
      intmax (type::integral::from_untyped(intmax_type_node)),
      uintmax(type::integral::from_untyped(uintmax_type_node)),
      
      const_char_ptr(type::pointer::from_untyped(const_string_type_node)),
      char_ptr      (type::pointer::from_untyped(string_type_node)),
      
      const_void_ptr(type::pointer::from_untyped(const_ptr_type_node)),
      void_ptr      (type::pointer::from_untyped(ptr_type_node)),
      
      intptr (type::integral::from_untyped(intptr_type_node)),
      uintptr(type::integral::from_untyped(uintptr_type_node)),
      ptrdiff(type::integral::from_untyped(ptrdiff_type_node)),
      size   (type::integral::from_untyped(size_type_node)),
      ssize  (type::integral::from_untyped(signed_size_type_node))
   {}
   
   type::integral builtin_types::smallest_integral_for(size_t bitcount, bool is_signed) const {
      if (bitcount > 32)
         return is_signed ? int64 : uint64;
      if (bitcount > 16)
         return is_signed ? int32 : uint32;
      if (bitcount > 8)
         return is_signed ? int16 : uint16;
      return is_signed ? int8 : uint8;
   }
}