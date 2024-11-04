#pragma once
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/integral.h"
#include "gcc_wrappers/type/pointer.h"
#include "lu/singleton.h"

namespace gcc_wrappers {
   class builtin_types;
   class builtin_types : public lu::singleton<builtin_types> {
      protected:
         builtin_types();
   
      public:
         type::base     basic_bool;
         type::base     basic_char;
         type::integral basic_int;
         type::base     basic_void;
         type::integral int8;
         type::integral int16;
         type::integral int32;
         type::integral int64;
         type::integral uint8;
         type::integral uint16;
         type::integral uint32;
         type::integral uint64;
         
         type::integral intmax;
         type::integral uintmax;
         
         type::pointer  const_char_ptr; // const char*
         type::pointer  char_ptr; // char*
         
         type::pointer  const_void_ptr; // const void*
         type::pointer  void_ptr; // void*
         
         type::integral intptr;
         type::integral uintptr;
         type::integral ptrdiff;
         type::integral size;   // size_t
         type::integral ssize;  // ssize_t (signed size_t)
         
      public:
         type::integral smallest_integral_for(size_t bitcount, bool is_signed = false) const;
   };
}