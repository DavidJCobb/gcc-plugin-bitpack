
/*

   TESTCASE: Splitting a struct across sector boundaries.

*/

#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool
#include <stdio.h>
#include <string.h> // memset

#define LU_BITCOUNT(n) __attribute__((lu_bitpack_bitcount(n)))

#pragma lu_bitpack set_options ( \
   sector_count=3, \
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

static struct TestStruct {
   bool8 a;
   bool8 b;
   bool8 c;
   bool8 d;
   bool8 e;
   LU_BITCOUNT(5)  u8  f;
   LU_BITCOUNT(3)  u8  g;
   LU_BITCOUNT(12) u16 h;
   LU_BITCOUNT(24) u32 i;
   LU_BITCOUNT(24) u32 j;
   LU_BITCOUNT(24) u32 k;
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
   printf("   }\n");
}

int main() {
   u8 sector_0_buffer[64] = { 0 };
   u8 sector_1_buffer[64] = { 0 };
   u8 sector_2_buffer[64] = { 0 };
   
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
   
   memset(sector_0_buffer, '~', sizeof(sector_0_buffer));
   memset(sector_1_buffer, '~', sizeof(sector_1_buffer));
   memset(sector_2_buffer, '~', sizeof(sector_2_buffer));
   
   printf(divider);
   printf("Test data to save:\n");
   printf(divider);
   print_data();
   printf("\n");
   printf(divider);
   printf("Testing generated functions...\n");
   printf(divider);
   
   generated_save(sector_0_buffer, 0);
   generated_save(sector_1_buffer, 1);
   generated_save(sector_2_buffer, 2);
   memset(&sTestStruct, 0, sizeof(sTestStruct));
   
   printf("Sector 0 saved:\n");
   print_buffer(sector_0_buffer, sizeof(sector_0_buffer));
   printf("Sector 0 read:\n");
   generated_read(sector_0_buffer, 0);
   print_data();
   
   printf("Sector 1 saved:\n");
   print_buffer(sector_1_buffer, sizeof(sector_1_buffer));
   printf("Sector 1 read:\n");
   generated_read(sector_1_buffer, 1);
   print_data();
   
   printf("Sector 2 saved:\n");
   print_buffer(sector_2_buffer, sizeof(sector_2_buffer));
   printf("Sector 2 read:\n");
   generated_read(sector_2_buffer, 2);
   print_data();
   
   return 0;
}