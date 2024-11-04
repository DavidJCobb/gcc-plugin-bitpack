
/*

   TESTCASE: Heritable options.

*/

#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool
#include <stdio.h>
#include <string.h> // memset

#define LU_BP_OPAQUE __attribute__((lu_bitpack_as_opaque_buffer))
#define LU_BP_INHERIT(name) __attribute__((lu_bitpack_inherit(name)))

#pragma lu_bitpack set_options ( \
   sector_count=1, \
   sector_size=16,  \
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

#pragma lu_bitpack heritable string "$player-name" ( length=7, with_terminator )
#pragma lu_bitpack heritable string "$creature-name" ( length=10 )
#pragma lu_bitpack heritable integer "$language" ( max=7 )

static struct TestStruct {
   LU_BP_INHERIT("$player-name")   u8 playerName[8];
   LU_BP_INHERIT("$creature-name") u8 creatureName[10];
   LU_BP_INHERIT("$language")      u8 language;
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
   printf("      .playerName = %s,\n", sTestStruct.playerName);
   {
      char str[11];
      memcpy(str, sTestStruct.creatureName, 10);
      str[10] = '\0';
      printf("      .creatureName = %s,\n", str);
   }
   printf("      .language = %u,\n", sTestStruct.language);
   printf("   }\n");
}

int main() {
   u8 test_buffer[64] = { 0 };
   
   const char* divider = "=========================================================\n";
   
   printf(divider);
   printf("Test: optionally-terminated string at full length\n");
   printf(divider);
   printf("\n");
   
   memcpy(sTestStruct.playerName,   "Lucy", 5);
   memcpy(sTestStruct.creatureName, "Fredericka", 10); // deliberately exclude null
   sTestStruct.language = 3;
   
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
   
   memset(test_buffer, 0, sizeof(test_buffer));
   memset(&sTestStruct, 0, sizeof(sTestStruct));
   printf("\n");
   printf(divider);
   printf("Test: optionally-terminated string shorter than buffer\n");
   printf(divider);
   printf("\n");
   
   memcpy(sTestStruct.playerName,   "Lucy", 5);
   memcpy(sTestStruct.creatureName, "Anna", 5);
   sTestStruct.language = 3;
   
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