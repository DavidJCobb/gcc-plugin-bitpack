
/*

   TESTCASE: Typedef on array

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

LU_BP_STRING_UT typedef u8 player_name_type [7];

LU_BP_BITCOUNT(3) typedef u8 small_numbers [3];

static struct TestStruct {
   LU_BP_DEFAULT("Abcdefg") player_name_type player_name;
   
   LU_BP_DEFAULT("Abcdefg") player_name_type enemy_names[5];
   
   small_numbers smalls;
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
   print_top_level_string_member("player_name", 7, sTestStruct.player_name);
   printf("      .enemy_names = {\n");
   for(int i = 0; i < 5; ++i) {
   printf("         [%u] = {\n", i);
   for(int j = 0; j < 7; ++j) {
   printf("            ");
   print_char(sTestStruct.enemy_names[i][j]);
   printf(",\n");
   }
   printf("         },\n");
   }
   printf("      },\n");
   printf("      .smalls = {\n");
   for(int i = 0; i < 3; ++i) {
   printf("         %u,\n", sTestStruct.smalls[i]);
   }
   printf("      },\n");
   printf("   }\n");
}

int main() {
   #define SECTOR_COUNT 2
   u8 sector_buffers[SECTOR_COUNT][64] = { 0 };
   
   const char* divider = "=========================================================\n";
   
   for(int i = 0; i < 7; ++i) {
      sTestStruct.player_name[i] = 'A' + i;
   }
   for(int i = 0; i < 5; ++i) {
      for(int j = 0; j < 7; ++j) {
         int c = (i * 7) + j;
         c = c % 26;
         
         sTestStruct.enemy_names[i][j] = 'A' + c;
      }
   }
   for(int i = 0; i < 3; ++i) {
      sTestStruct.smalls[i] = (i + 1) * 2;
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