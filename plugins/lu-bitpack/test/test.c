
#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool
#include <stdio.h>

#define LU_BITCOUNT(n) __attribute__((lu_bitpack_bitcount(n)))
#define LU_STRING(...) __attribute__((lu_bitpack_string(__VA_ARGS__)))

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
   func_write_buffer = lu_BitstreamWrite_buffer, \
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

extern void save(u8* dst, int sector_id) {
   struct lu_BitstreamState state;
   lu_BitstreamInitialize(&state, dst);
   
   for(int i = 0; i < 9; ++i)
      lu_BitstreamWrite_u8(&state, sStructA.a[i], 6);
   
   lu_BitstreamWrite_u8(&state, sStructA.b, 5);
   
   lu_BitstreamWrite_string(&state, sStructB.a, 3);
}
//#pragma lu_bitpack debug_dump_function save

extern void read(const u8* src, int sector_id) {
   struct lu_BitstreamState state;
   lu_BitstreamInitialize(&state, (u8*)src);
   
   for(int i = 0; i < 9; ++i)
      sStructA.a[i] = lu_BitstreamRead_u8(&state, 6);
   
   sStructA.b = lu_BitstreamRead_u8(&state, 5);
   
   lu_BitstreamRead_string(&state, sStructB.a, 5);
}
#pragma lu_bitpack debug_dump_function read

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,              \
   save_name = generated_save,              \
   data      = sStructA sStructB | sStructC \
)
#pragma lu_bitpack debug_dump_function __lu_bitpack_read_sector_0


extern void _read_a(struct lu_BitstreamState* state, struct StructA* dst) {
   for(int i = 0; i < 9; ++i)
      dst->a[i] = lu_BitstreamRead_u8(state, 6);
   
   dst->b = lu_BitstreamRead_u8(state, 5);
}
extern void _read_b(struct lu_BitstreamState* state, struct StructB* dst) {
   lu_BitstreamRead_string(state, dst->a, 5);
}
extern void _read_c(struct lu_BitstreamState* state, struct StructC* dst) {
   lu_BitstreamRead_string(state, dst->a, 23);
}
extern void _read_sector_0(struct lu_BitstreamState* state) {
   _read_a(state, &sStructA);
   _read_b(state, &sStructB);
   _read_c(state, &sStructC);
}
#pragma lu_bitpack debug_dump_function _read_sector_0



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

void print_buffer(const u8* buffer, int size) {
   printf("Buffer: ");
   for(int i = 0; i < size; ++i) {
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
         printf("'\0' ");
      } else {
         printf("%c ", c);
      }
   }
   printf("\n");
   
   printf(" - sStructC:\n");
   printf("    - a == ");
   for(int i = 0; i < 24; ++i) {
      char c = sStructB.a[i];
      if (c == '\0') {
         printf("'\0' ");
      } else {
         printf("%c ", c);
      }
   }
   printf("\n");
}

int main() {
   u8 test_buffer[64] = { 0 };
   
   printf("Testing pre-written functions...\n\n");
   reset_data();
   
   save(test_buffer, 0);
   printf("Saved:\n");
   print_data();
   print_buffer(test_buffer, sizeof(test_buffer));
   
   read(test_buffer, 0);
   printf("Read:\n");
   print_data();
   print_buffer(test_buffer, sizeof(test_buffer));
   
   printf("Testing generated functions...\n\n");
   reset_data();
   
   generated_save(test_buffer, 0);
   printf("Saved:\n");
   print_data();
   print_buffer(test_buffer, sizeof(test_buffer));
   
   generated_read(test_buffer, 0);
   printf("Read:\n");
   print_data();
   print_buffer(test_buffer, sizeof(test_buffer));
   
   return 0;
}