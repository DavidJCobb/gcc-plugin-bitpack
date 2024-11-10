
/*

   TESTCASE: Arrays of opaque buffers.

*/

#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#include <stdbool.h> // bool
#include <string.h> // memset

#pragma lu_bitpack enable

#pragma lu_bitpack set_options ( \
   sector_count=9, \
   sector_size=64, \
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

union OpaqueUnion {
   struct {
      u8 pad[8];
   } foo;
};

static struct TestStruct {
   LU_BP_OMIT LU_BP_STRING_UT LU_BP_DEFAULT("ABCDEFG") u8 creatureName1[7];
   LU_BP_OMIT LU_BP_STRING_UT LU_BP_DEFAULT("ABCDE") u8 creatureName2[7];
   
   LU_BP_AS_OPAQUE_BUFFER union OpaqueUnion single;
   
   LU_BP_AS_OPAQUE_BUFFER union OpaqueUnion array[5];
} sTestStruct;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)

void print_data() {
   printf("   sTestStruct == {\n");
   print_top_level_string_member("creatureName1", 7, sTestStruct.creatureName1);
   print_top_level_string_member("creatureName2", 7, sTestStruct.creatureName2);
   printf("      .single = {\n");
   for(int i = 0; i < 8; ++i) {
      printf("         %u,\n", sTestStruct.single.foo.pad[i]);
   }
   printf("      },\n");
   printf("      .array = {\n");
   for(int i = 0; i < 5; ++i) {
      printf("         [%u] = {\n", i);
      for(int j = 0; j < 8; ++j) {
         printf("            %u,\n", sTestStruct.array[i].foo.pad[j]);
      }
      printf("         },\n");
   }
   printf("      },\n");
   printf("   }\n");
}

int main() {
   #define SECTOR_COUNT 2
   u8 sector_buffers[SECTOR_COUNT][64] = { 0 };
   
   const char* divider = "=========================================================\n";
   
   for(int i = 0; i < 7; ++i) {
      sTestStruct.creatureName1[i] = 'A' + i;
   }
   for(int i = 0; i < 7; ++i) {
      sTestStruct.creatureName2[i] = 'A' + i;
   }
   for(int i = 0; i < 8; ++i) {
      sTestStruct.single.foo.pad[i] = i;
   }
   for(int i = 0; i < 5; ++i) {
      for(int j = 0; j < 8; ++j) {
         sTestStruct.array[i].foo.pad[j] = i * 10 + j;
      }
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