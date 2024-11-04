
/*

   TESTCASE: Opaque buffers.

*/

#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool
#include <stdio.h>
#include <string.h> // memset

#define LU_BP_OPAQUE __attribute__((lu_bitpack_as_opaque_buffer))

#pragma lu_bitpack set_options ( \
   sector_count=1, \
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
   LU_BP_OPAQUE union {
      u8    foo;
      float bar;
   } a;
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
   printf("      .a.foo = %u,\n", sTestStruct.a.foo);
   printf("      .a.bar = %f,\n", sTestStruct.a.bar);
   printf("   }\n");
}

int main() {
   u8 test_buffer[64] = { 0 };
   
   const char* divider = "=========================================================\n";
   
   sTestStruct.a.bar = 1.0F;
   
   printf(divider);
   printf("Test data to save:\n");
   printf(divider);
   print_data();
   printf("\n");
   printf(divider);
   printf("Testing generated functions...\n");
   printf(divider);
   
   generated_save(test_buffer, 0);
   memset(&sTestStruct, 0, sizeof(sTestStruct));
   
   printf("Sector 0 saved:\n");
   print_buffer(test_buffer, sizeof(test_buffer));
   printf("Sector 0 read:\n");
   generated_read(test_buffer, 0);
   print_data();
   
   return 0;
}