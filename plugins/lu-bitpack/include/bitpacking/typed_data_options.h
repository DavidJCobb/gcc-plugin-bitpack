#pragma once

namespace bitpacking::typed_data_options {
   struct buffer {
      size_t bytecount = 0;
   };
   struct integral { // also boolean, pointer
      size_t    bitcount = 0;
      intmax_t  min = 0;
      uintmax_t max = 0;
   };
   struct string {
      size_t length = 0;
      bool   with_terminator = false;
   };
}