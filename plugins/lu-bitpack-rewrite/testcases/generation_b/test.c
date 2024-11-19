
//
// Test-case: top-level VAR_DECLs that are not struct or union types.
//

#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#define SECTOR_COUNT 2
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

static int sTestInt = 0;
LU_BP_STRING_UT static char sTestString[5];
LU_BP_OMIT LU_BP_STRING_UT LU_BP_DEFAULT("VWXYZ") static char sTestOmittedString[5];
static struct TestStruct {
   int a[2];
} sTestStruct;
//#pragma lu_bitpack debug_dump_as_serialization_item sTestInt
//#pragma lu_bitpack debug_dump_as_serialization_item sTestStruct

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestInt sTestString sTestOmittedString sTestStruct \
)
//#pragma lu_bitpack debug_dump_function generated_read
//#pragma lu_bitpack debug_dump_function __lu_bitpack_read_sector_0
//#pragma lu_bitpack debug_dump_function __lu_bitpack_save_TestStruct

#include <string.h> // memset

void print_test_data() {
   printf("sTestInt == %d\n", sTestInt);
   
   printf("sTestString == { ");
   for(int i = 0; i < sizeof(sTestString); ++i) {
      print_char(sTestString[i]);
      if (i + 1 < sizeof(sTestString))
         printf(", ");
   }
   printf(" }\n");
   
   printf("sTestOmittedString == { ");
   for(int i = 0; i < sizeof(sTestOmittedString); ++i) {
      print_char(sTestOmittedString[i]);
      if (i + 1 < sizeof(sTestOmittedString))
         printf(", ");
   }
   printf(" }\n");
   
   printf("sTestStruct == {\n");
   printf("   .a == { %d, %d },\n", sTestStruct.a[0], sTestStruct.a[1]);
   printf("}\n");
}

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sTestInt = 12345;
   memcpy(&sTestString, "ABCDE", 6);
   memcpy(&sTestOmittedString, "XXXXX", 6);
   for(int i = 0; i < 2; ++i) {
      sTestStruct.a[i] = i + 1;
   }
   
   const char* divider = "====================================================\n";
   
   printf(divider);
   printf("Test data:\n");
   printf(divider);
   print_test_data();
   printf("\n");
   
   //
   // Perform save.
   //
   memset(&sector_buffers, '~', sizeof(sector_buffers));
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      generated_save(sector_buffers[i], i);
   }
   memset(&sTestInt,    0xCC, sizeof(sTestInt));
   memset(&sTestString, 0xCC, sizeof(sTestString));
   memset(&sTestOmittedString, 0xCC, sizeof(sTestOmittedString));
   memset(&sTestStruct, 0xCC, sizeof(sTestStruct));
   
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      printf("Sector %u saved:\n", i);
      print_buffer(sector_buffers[i], sizeof(sector_buffers[i]));
      printf("Sector %u read:\n", i);
      generated_read(sector_buffers[i], i);
      print_test_data();
   }
   
   
   return 0;
}