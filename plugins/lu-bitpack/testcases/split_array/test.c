
/*

   TESTCASE: Split an array across sector boundaries.

*/

#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool
#include <stdio.h>
#include <string.h> // memset

#define LU_BITCOUNT(n) __attribute__((lu_bitpack_bitcount(n)))
#define LU_STRING(...) __attribute__((lu_bitpack_string(__VA_ARGS__)))

#pragma lu_bitpack set_options ( \
   sector_count=2, \
   sector_size=2,  \
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
   LU_BITCOUNT(8) u8 a[3];
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
   printf("      .a = {\n");
   for(int i = 0; i < 3; ++i) {
      printf("         %u,\n", sTestStruct.a[i]);
   }
   printf("      }\n");
   printf("   }\n");
}

int main() {
   u8 sector_0_buffer[64] = { 0 };
   u8 sector_1_buffer[64] = { 0 };
   
   const char* divider = "=========================================================\n";
   
   sTestStruct.a[0] = 0x11;
   sTestStruct.a[1] = 0x22;
   sTestStruct.a[2] = 0x33;
   
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
   memset(&sTestStruct, 0, sizeof(sTestStruct));
   
   printf("Sector 0 saved:\n");
   print_buffer(sector_0_buffer, sizeof(sector_0_buffer));
   //
   printf("Sector 0 read:\n");
   generated_read(sector_0_buffer, 0);
   print_data();
   
   printf("Sector 1 saved:\n");
   print_buffer(sector_1_buffer, sizeof(sector_1_buffer));
   //
   printf("Sector 1 read:\n");
   generated_read(sector_1_buffer, 1);
   print_data();
   
   return 0;
}