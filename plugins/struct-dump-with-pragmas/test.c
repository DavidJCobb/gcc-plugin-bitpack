
#include <stdint.h>
typedef uint8_t bool8;

typedef char char_t;

struct FooStruct {
   __attribute__((lu_bitpack_string("with-terminator, length = 7 "))) char x[7];
   __attribute__((lu_bitpack_bitcount(30))) int32_t a;
   __attribute__((lu_bitpack_bitcount(1))) bool8 b;
   __attribute__((lu_bitpack_string("with-terminator, length =7"))) char c[7];
   struct {
      int32_t e;
   } d;
   union {
      int32_t f;
      char    g[4];
   };
   void* h;
   const int i;
   int j : 5;
   __attribute__((lu_bitpack_omit)) int k : 3;
   int l : 17;
   __attribute__((lu_bitpack_bitcount(70))) int m[4][3][2];
   
   char    array_of_char[7];
   uint8_t array_of_u8[7];
   int8_t  array_of_s8[7];
   bool8   array_of_bool[7];
   char_t array_of_typedef_char[7];
   
   __attribute__((lu_bitpack_string)) int8_t  not_string_s[7];
   __attribute__((lu_bitpack_string)) uint8_t not_string_u[7];
   __attribute__((lu_bitpack_string)) char_t  string_via_typedef[7];
};

// this reads as a nameless struct.
typedef struct {
   int a;
} BarStruct;