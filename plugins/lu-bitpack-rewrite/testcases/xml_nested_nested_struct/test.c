
#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#define SECTOR_COUNT 1
#define SECTOR_SIZE 16

#pragma lu_bitpack enable
#pragma lu_bitpack set_options ( \
   sector_count=SECTOR_COUNT, \
   sector_size=SECTOR_SIZE,  \
   bool_typename            = bool8, \
   buffer_byte_typename     = void, \
   bitstream_state_typename = lu_BitstreamState, \
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

#define TEST_ARRAYS 1
#define TEST_UNIONS 1

struct B {
   LU_BP_CATEGORY("category-a") LU_BP_CATEGORY("category-b") int data;
};
struct A {
   struct B onion;
};

static struct TestStruct {
   struct A wrapped;
} sTestStruct;
#pragma lu_bitpack debug_dump_as_serialization_item sTestStruct

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)
//#pragma lu_bitpack debug_dump_function __lu_bitpack_read_sector_2

#include <string.h> // memset

void print_test_struct() {
   printf("sTestStruct == {\n");
   printf("   .wrapped = {\n");
   printf("      .onion = {\n");
   printf("         .data = %d,\n", sTestStruct.wrapped.onion.data);
   printf("      },\n");
   printf("   },\n");
   printf("}\n");
}

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sTestStruct.wrapped.onion.data = 55055055;
   
   const char* divider = "====================================================\n";
   
   printf(divider);
   printf("Test data:\n");
   printf(divider);
   print_test_struct();
   printf("\n");
   
   //
   // Perform save.
   //
   memset(&sector_buffers, '~', sizeof(sector_buffers));
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      printf("Saving sector %u...\n", i);
      generated_save(sector_buffers[i], i);
   }
   printf("All sectors saved. Wiping sTestStruct...\n");
   memset(&sTestStruct, 0xCC, sizeof(sTestStruct));
   printf("Wiped. Reviewing data...\n\n");
   
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      printf("Sector %u saved:\n", i);
      print_buffer(sector_buffers[i], sizeof(sector_buffers[i]));
      printf("Sector %u read:\n", i);
      generated_read(sector_buffers[i], i);
      print_test_struct();
   }
   
   
   return 0;
}