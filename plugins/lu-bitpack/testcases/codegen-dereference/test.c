
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
   func_read_string_ut = lu_BitstreamRead_string_optional_terminator, \
   func_read_string_nt = lu_BitstreamRead_string, \
   func_read_buffer = lu_BitstreamRead_buffer, \
   func_write_bool   = lu_BitstreamWrite_bool, \
   func_write_u8     = lu_BitstreamWrite_u8,   \
   func_write_u16    = lu_BitstreamWrite_u16,  \
   func_write_u32    = lu_BitstreamWrite_u32,  \
   func_write_s8     = lu_BitstreamWrite_s8,   \
   func_write_s16    = lu_BitstreamWrite_s16,  \
   func_write_s32    = lu_BitstreamWrite_s32,  \
   func_write_string_ut = lu_BitstreamWrite_string_optional_terminator, \
   func_write_string_nt = lu_BitstreamWrite_string, \
   func_write_buffer = lu_BitstreamWrite_buffer \
)

struct TestStruct {
   LU_BP_BITCOUNT(3)    int a;
   LU_BP_MINMAX(0, 220) int b;
   int c;
   LU_BP_BITCOUNT(7) u8 d[3];
};

struct TestStruct* sTestStructPtr;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = *sTestStructPtr,        \
   enable_debug_output = true \
)
//#pragma lu_bitpack debug_dump_function generated_read

#include <string.h> // memset

void print_test_struct() {
   printf("*sTestStructPtr == {\n");
   printf("   .a == %d\n", sTestStructPtr->a);
   printf("   .b == %d\n", sTestStructPtr->b);
   printf("   .c == %d\n", sTestStructPtr->c);
   printf("   .d == { %d, %d, %d },\n", sTestStructPtr->d[0], sTestStructPtr->d[1], sTestStructPtr->d[2]);
   printf("}\n");
}

#include <stdlib.h> // malloc

int main() {
   sTestStructPtr = (struct TestStruct*) malloc(sizeof(struct TestStruct));
   
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sTestStructPtr->a = 2;
   sTestStructPtr->b = 219;
   sTestStructPtr->c = 5550555;
   sTestStructPtr->d[0] = 127;
   sTestStructPtr->d[1] = 64;
   sTestStructPtr->d[2] = 96;
   
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
   printf("All sectors saved. Wiping sTestStructPtr...\n");
   memset(sTestStructPtr, 0xCC, sizeof(struct TestStruct));
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