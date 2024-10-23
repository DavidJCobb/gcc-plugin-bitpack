
#include <stdint.h>
typedef uint8_t bool8;

struct FooStruct {
   int32_t a;
   bool8   b;
   char    c[7];
   struct {
      int32_t e;
   } d;
};
