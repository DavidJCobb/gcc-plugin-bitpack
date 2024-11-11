#ifndef GUARD_LU_TEST_HELPERS
#define GUARD_LU_TEST_HELPERS

#define LU_BP_AS_OPAQUE_BUFFER __attribute__((lu_bitpack_as_opaque_buffer))
#define LU_BP_BITCOUNT(n)      __attribute__((lu_bitpack_bitcount(n)))
#define LU_BP_DEFAULT(x)       __attribute__((lu_bitpack_default_value(x)))
#define LU_BP_OMIT             __attribute__((lu_bitpack_omit))
#define LU_BP_STRING           __attribute__((lu_bitpack_string))
#define LU_BP_STRING_NT        LU_BP_STRING
#define LU_BP_STRING_UT        __attribute__((lu_nonstring)) LU_BP_STRING
#define LU_BP_TRANSFORM(pre_pack, post_unpack) \
   __attribute__((lu_bitpack_transforms("pre_pack="#pre_pack",post_unpack="#post_unpack)))

#include <ctype.h>
#include <stdio.h>

void print_buffer(const char* buffer, int size) {
   printf("Buffer:\n   ");
   for(int i = 0; i < size; ++i) {
      if (i && i % 8 == 0) {
         printf("\n   ");
      }
      printf("%02X ", buffer[i]);
   }
   printf("\n");
}

void print_char(char c) {
   if (isprint(c)) {
      printf("'%c'", c & 0xFF);
   } else {
      printf("0x%02X", c & 0xFF);
   }
}

void print_top_level_string_member(const char* name, int size, const char* data) {
   printf("      .%s = {\n", name);
   for(int i = 0; i < size; ++i) {
      printf("         ");
      print_char(data[i]);
      printf(",\n");
   }
   printf("      },\n");
}

#endif