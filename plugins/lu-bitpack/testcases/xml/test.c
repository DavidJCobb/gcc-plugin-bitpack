
/*

   TESTCASE: Generating an XML report of the bitpack format.

*/

#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool
#include <stdio.h>
#include <string.h> // memset

#pragma lu_bitpack enable

#if __GNUC__ >= 8
   #define NONSTRING __attribute__((nonstring))
#else
   #define NONSTRING 
#endif

#define LU_BP_BITCOUNT(n)   __attribute__((lu_bitpack_bitcount(n)))
#define LU_BP_DEFAULT(x)    __attribute__((lu_bitpack_default_value(x)))
#define LU_BP_INHERIT(name) __attribute__((lu_bitpack_inherit(name)))
#define LU_BP_OMIT          __attribute__((lu_bitpack_omit))
#define LU_BP_STRING        __attribute__((lu_bitpack_string))
#define LU_BP_STRING_NT     LU_BP_STRING
#define LU_BP_STRING_UT     NONSTRING LU_BP_STRING

#pragma lu_bitpack set_options ( \
   sector_count=9, \
   sector_size=8,  \
   bool_typename            = bool8, \
   buffer_byte_typename     = void, \
   bitstream_state_typename = lu_BitstreamState, \
   string_char_typename     = u8, \
   func_initialize  = lu_BitstreamInitialize, \
   func_read_bool   = lu_BitstreamRead_bool, \
   func_read_u8     = lu_BitstreamRead_u8,   \
   func_read_u16    = lu_BitstreamRead_u16,  \
   func_read_u32    = lu_BitstreamRead_u32,  \
   func_read_s8     = lu_BitstreamRead_s8,   \
   func_read_s16    = lu_BitstreamRead_s16,  \
   func_read_s32    = lu_BitstreamRead_s32,  \
   func_read_string = lu_BitstreamRead_string_optional_terminator, \
   func_read_string_terminated = lu_BitstreamRead_string, \
   func_read_buffer = lu_BitstreamRead_buffer, \
   func_write_bool   = lu_BitstreamWrite_bool, \
   func_write_u8     = lu_BitstreamWrite_u8,   \
   func_write_u16    = lu_BitstreamWrite_u16,  \
   func_write_u32    = lu_BitstreamWrite_u32,  \
   func_write_s8     = lu_BitstreamWrite_s8,   \
   func_write_s16    = lu_BitstreamWrite_s16,  \
   func_write_s32    = lu_BitstreamWrite_s32,  \
   func_write_string = lu_BitstreamWrite_string_optional_terminator, \
   func_write_string_terminated = lu_BitstreamWrite_string, \
   func_write_buffer = lu_BitstreamWrite_buffer \
)

#pragma lu_bitpack heritable integer "$24bit" ( bitcount = 24 )

static struct TestStruct {
   bool8 a;
   bool8 b;
   bool8 c;
   bool8 d;
   bool8 e;
   LU_BP_BITCOUNT(5)       u8  f;
   LU_BP_BITCOUNT(3)       u8  g;
   LU_BP_BITCOUNT(12)      u16 h;
   LU_BP_INHERIT("$24bit") u32 i;
   LU_BP_INHERIT("$24bit") u32 j;
   LU_BP_BITCOUNT(24)      u32 k;
   LU_BP_BITCOUNT(7)       u8  l[7];
   LU_BP_STRING            u8  name[6];
   
   LU_BP_OMIT LU_BP_DEFAULT(1.5)   float defaulted_f;
   LU_BP_OMIT LU_BP_DEFAULT(7)     u8    defaulted_i;
   LU_BP_OMIT LU_BP_DEFAULT("foo") u8    defaulted_s[4];
   
   LU_BP_OMIT LU_BP_STRING_UT LU_BP_DEFAULT("ABCDEFG") u8 creatureName1[7];
   LU_BP_OMIT LU_BP_STRING_UT LU_BP_DEFAULT("ABCDE") u8 creatureName2[7];
   
   LU_BP_BITCOUNT((1 + 2) / 3) u8 math_single_bit;
} sTestStruct;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)

void print_buffer(const u8* buffer, int size) {
   printf("Buffer:\n   ");
   for(int i = 0; i < size; ++i) {
      if (i && i % 8 == 0) {
         printf("\n   ");
      }
      printf("%02X ", buffer[i]);
   }
   printf("\n");
}
#include <ctype.h>
void print_char(char c) {
   if (isprint(c)) {
      printf("'%c'", c);
   } else {
      printf("0x%02X", c);
   }
}
void print_data() {
   printf("   sTestStruct == {\n");
   printf("      .a = %u,\n", sTestStruct.a);
   printf("      .b = %u,\n", sTestStruct.b);
   printf("      .c = %u,\n", sTestStruct.c);
   printf("      .d = %u,\n", sTestStruct.d);
   printf("      .e = %u,\n", sTestStruct.e);
   printf("      .f = %u,\n", sTestStruct.f);
   printf("      .g = %u,\n", sTestStruct.g);
   printf("      .h = %u,\n", sTestStruct.h);
   printf("      .i = %u,\n", sTestStruct.i);
   printf("      .j = %u,\n", sTestStruct.j);
   printf("      .k = %u,\n", sTestStruct.k);
   printf("      .l = {\n");
   for(int i = 0; i < 7; ++i) {
      printf("         %u,\n", sTestStruct.l[i]);
   }
   printf("      },\n");
   printf("      .name = {\n");
   for(int i = 0; i < 6; ++i) {
      printf("         ");
      print_char(sTestStruct.name[i]);
      printf(",\n");
   }
   printf("      },\n");
   printf("      .defaulted_f = %f,\n", sTestStruct.defaulted_f);
   printf("      .defaulted_i = %u,\n", sTestStruct.defaulted_i);
   printf("      .defaulted_s = {\n");
   for(int i = 0; i < 4; ++i) {
      printf("         ");
      print_char(sTestStruct.defaulted_s[i]);
      printf(",\n");
   }
   printf("      },\n");
   printf("      .creatureName1 = {\n");
   for(int i = 0; i < 7; ++i) {
      printf("         ");
      print_char(sTestStruct.creatureName1[i]);
      printf(",\n");
   }
   printf("      },\n");
   printf("      .creatureName2 = {\n");
   for(int i = 0; i < 7; ++i) {
      printf("         ");
      print_char(sTestStruct.creatureName2[i]);
      printf(",\n");
   }
   printf("      },\n");
   printf("   }\n");
}

int main() {
   #define SECTOR_COUNT 6
   u8 sector_buffers[SECTOR_COUNT][64] = { 0 };
   
   const char* divider = "=========================================================\n";
   
   sTestStruct.a = true;
   sTestStruct.b = false;
   sTestStruct.c = true;
   sTestStruct.d = false;
   sTestStruct.e = true;
   sTestStruct.f = 0b11111;
   sTestStruct.g = 0b101;
   sTestStruct.h = 7;
   sTestStruct.i = 0b10101010101010;
   sTestStruct.j = 0b10101010101010;
   sTestStruct.k = 0b10101010101010;
   for(int i = 0; i < 7; ++i) {
      sTestStruct.l[i] = (i + 1) * 11;
   }
   for(int i = 0; i < 7; ++i) {
      sTestStruct.creatureName1[i] = 'A' + i;
   }
   for(int i = 0; i < 7; ++i) {
      sTestStruct.creatureName2[i] = 'A' + i;
   }
   
   memset(&sector_buffers, '~', sizeof(sector_buffers));
   
   printf(divider);
   printf("Test data to save:\n");
   printf(divider);
   print_data();
   printf("\n");
   printf(divider);
   printf("Testing generated functions...\n");
   printf(divider);
   
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      generated_save(sector_buffers[i], i);
   }
   memset(&sTestStruct, 0xCC, sizeof(sTestStruct));
   
   
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      printf("Sector %u saved:\n", i);
      print_buffer(sector_buffers[i], sizeof(sector_buffers[i]));
      printf("Sector %u read:\n", i);
      generated_read(sector_buffers[i], i);
      print_data();
   }
   
   return 0;
}