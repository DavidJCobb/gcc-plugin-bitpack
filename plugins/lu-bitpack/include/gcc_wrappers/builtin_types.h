#pragma once
#include "gcc_wrappers/type.h"
#include "lu/singleton.h"

namespace gcc_wrappers {
   class builtin_types;
   class builtin_types : public lu::singleton<builtin_types> {
      protected:
         builtin_types();
   
      public:
         type basic_bool;
         type basic_char;
         type basic_int;
         type basic_void;
         type int8;
         type int16;
         type int32;
         type int64;
         type uint8;
         type uint16;
         type uint32;
         type uint64;
         
         type intmax;
         type uintmax;
         
         type const_char_ptr; // const char*
         type char_ptr; // char*
         
         type const_void_ptr; // const void*
         type void_ptr; // void*
         
         type intptr;
         type uintptr;
         type ptrdiff;
         type size;   // size_t
         type ssize;  // ssize_t (signed size_t)
         
      public:
         type smallest_integral_for(size_t bitcount, bool is_signed = false) const;
   };
}