
/*

   TESTCASE: Bitpacking options and struct members are arranged such that 
   a string-type member is too large to fit inside of a single sector.

*/

#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool
#include <stdio.h>
#include <string.h> // memset

#define LU_BITCOUNT(n) __attribute__((lu_bitpack_bitcount(n)))
#define LU_STRING(...) __attribute__((lu_bitpack_string(__VA_ARGS__)))

#pragma lu_bitpack set_options ( \
   sector_count=3, \
   sector_size=3,  \
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
#define char u8

static struct StructA {
   LU_BITCOUNT(6) u8 a[9]; // bitcount is set per-element
   LU_BITCOUNT(5) u8 b;
} sStructA;
static struct StructB {
   LU_STRING("with-terminator") char a[6];
} sStructB;

static struct StructC {
   LU_STRING("with-terminator") char a[24];
} sStructC;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,              \
   save_name = generated_save,              \
   data      = sStructA sStructB | sStructC \
)

void reset_data() {
   for(int i = 0; i < 9; ++i)
      sStructA.a[i] = i + 2;
   sStructA.b = 3;
   
   for(int i = 0; i < 6; ++i)
      sStructB.a[i] = ("ABCDE\0")[i];
   
   for(int i = 0; i < 24; ++i) {
      if (i <= 10) {
         sStructC.a[i] = ("abcdefghij")[i];
      } else {
         sStructC.a[i] = 0;
      }
   }
}
void clear_data(int sector_id) {
   if (sector_id == 0) {
      memset(&sStructA, '~', sizeof(sStructA));
      memset(&sStructB, '~', sizeof(sStructB));
   } else if (sector_id == 1) {
      memset(&sStructC, '~', sizeof(sStructC));
   }
}

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
   printf(" - sStructA:\n");
   printf("    - a == ");
   for(int i = 0; i < 9; ++i) {
      printf("%02X ", sStructA.a[i]);
   }
   printf("\n");
   printf("    - b == %02X\n", sStructA.b);
   
   printf(" - sStructB:\n");
   printf("    - a == ");
   for(int i = 0; i < 6; ++i) {
      char c = sStructB.a[i];
      if (c == '\0') {
         printf("'\\0' ");
      } else {
         printf("%c ", c);
      }
   }
   printf("\n");
   
   printf(" - sStructC:\n");
   printf("    - a == ");
   for(int i = 0; i < 24; ++i) {
      char c = sStructC.a[i];
      if (c == '\0') {
         printf("'\\0' ");
      } else {
         printf("%c ", c);
      }
   }
   printf("\n");
}

int main() {
   u8 sector_0_buffer[64] = { 0 };
   u8 sector_1_buffer[64] = { 0 };
   u8 test_buffer[64] = { 0 };
   
   const char* divider = "=========================================================\n";
   
   printf(divider);
   printf("Test data to save:\n");
   printf(divider);
   reset_data();
   print_data();
   printf("\n");
   
   printf("\n");
   printf(divider);
   printf("Testing generated functions...\n");
   printf(divider);
   reset_data();
   printf("Sector 0 saved:\n");
   generated_save(sector_0_buffer, 0);
   print_buffer(sector_0_buffer, sizeof(sector_0_buffer));
   printf("Sector 0 read:\n");
   clear_data(0);
   generated_read(sector_0_buffer, 0);
   print_data();
   printf("Sector 1 saved:\n");
   generated_save(sector_1_buffer, 1);
   print_buffer(sector_1_buffer, sizeof(sector_1_buffer));
   printf("Sector 1 read:\n");
   clear_data(1);
   generated_read(sector_1_buffer, 1);
   print_data();
   
   return 0;
}