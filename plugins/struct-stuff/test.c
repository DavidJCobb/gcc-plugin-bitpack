
#include <stdint.h>
typedef uint8_t bool8;

struct FooStruct {
   char    x[7];
   int32_t a;
   bool8   b;
   char    c[7];
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
   int k : 3;
   int l : 17;
   int m[4][3][2];
};

// this reads as a nameless struct.
typedef struct {
   int a;
} BarStruct;