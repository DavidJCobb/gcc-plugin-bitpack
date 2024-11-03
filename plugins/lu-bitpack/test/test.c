
#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool

#define LU_BITCOUNT(n) __attribute__((lu_bitpack_bitcount(n)))
#define LU_STRING(...) __attribute__((lu_bitpack_string(__VA_ARGS__)))

#pragma lu_bitpack set_options ( \
   sector_count=3, \
   sector_size=8,  \
   bool_typename            = bool8, \
   buffer_byte_typename     = void,    \
   bitstream_state_typename = lu_BitstreamState, \
   string_char_typename     = u8,    \
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

//#pragma lu_bitpack debug_dump_identifier bool
//#pragma lu_bitpack debug_dump_identifier uint8_t
//#pragma lu_bitpack debug_dump_identifier char
//#pragma lu_bitpack debug_dump_identifier lu_BitstreamState

static struct StructA {
   LU_BITCOUNT(6) u8 a[9]; // bitcount is set per-element
   LU_BITCOUNT(5) u8 b;
} sStructA;
static struct StructB {
   LU_STRING("with-terminator") char a[6];
} sStructB;
//#pragma lu_bitpack debug_dump_identifier StructA
//#pragma lu_bitpack debug_dump_identifier sStructA

//typedef struct StructA test_typedef_StructA;
//#pragma lu_bitpack debug_dump_identifier test_typedef_StructA

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
#pragma lu_bitpack debug_dump_function save

extern void read(const u8* src, int sector_id) {
   struct lu_BitstreamState state;
   lu_BitstreamInitialize(&state, (u8*)src);
   
   for(int i = 0; i < 9; ++i)
      sStructA.a[i] = lu_BitstreamRead_u8(&state, 6);
   
   sStructA.b = lu_BitstreamRead_u8(&state, 5);
   
   lu_BitstreamRead_string(&state, sStructB.a, 5);
}
#pragma lu_bitpack debug_dump_function read

extern void read_generated(const u8* src, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = read_generated,              \
   save_name = save_generated,              \
   data      = sStructA sStructB | sStructC \
)
#pragma lu_bitpack debug_dump_function read_generated

extern void trigger_read() {
   u8 buffer[20];
   
   for(int i = 0; i < 9; ++i)
      buffer[i] = i + 2;
   buffer[9] = 3;
   
   buffer[10] = 'A';
   buffer[11] = 'B';
   buffer[12] = 'C';
   buffer[13] = 'D';
   buffer[14] = 'E';
   buffer[15] = '\0';
   
   read(buffer, 0);
}
extern void trigger_save() {
   u8 buffer[20];
   
   save(buffer, 0);
}

extern void do_sector_0();
extern void do_sector_1();
extern void do_sector_2();
extern void do_sector_3();
extern void test_if_else(int sector_id) {
   if (sector_id == 0) {
      do_sector_0();
   } else if (sector_id == 1) {
      do_sector_1();
   } else if (sector_id == 2) {
      do_sector_2();
   } else if (sector_id == 3) {
      do_sector_3();
   }
}
//#pragma lu_bitpack debug_dump_function test_if_else
   
struct TestPtrAccess {
   int a;
};
extern int test_ptr_access_func(struct TestPtrAccess* mut, const struct TestPtrAccess* immut) {
   mut->a = 5;
   
   int c = immut->a;
   return c + 3;
}
//#pragma lu_bitpack debug_dump_function test_ptr_access_func