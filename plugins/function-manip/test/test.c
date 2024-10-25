
#include "types.h"
#include "bitstreams.h"

#define LU_BITCOUNT(n) __attribute__((lu_bitpack_bitcount(n)))
#define LU_STRING(...) __attribute__((lu_bitpack_string(__VA_ARGS__)))

#pragma lu_bitpack set_options ( \
   sector_count=3, \
   sector_size=8,  \
   bool_typename            = bool, \
   buffer_byte_typename     = u8,   \
   bitstream_state_typename = lu_BitstreamState, \
   func_read_bool   = lu_BitstreamRead_bool, \
   func_read_u8     = lu_BitstreamRead_u8,   \
   func_read_u16    = lu_BitstreamRead_u16,  \
   func_read_u32    = lu_BitstreamRead_u32,  \
   func_read_s8     = lu_BitstreamRead_s8,   \
   func_read_s16    = lu_BitstreamRead_s16,  \
   func_read_s32    = lu_BitstreamRead_s32,  \
   func_read_string = lu_BitstreamRead_string_optional_terminator, \
   func_read_string_terminated = lu_BitstreamRead_string, \
   func_write_bool   = lu_BitstreamWrite_bool, \
   func_write_u8     = lu_BitstreamWrite_u8,   \
   func_write_u16    = lu_BitstreamWrite_u16,  \
   func_write_u32    = lu_BitstreamWrite_u32,  \
   func_write_s8     = lu_BitstreamWrite_s8,   \
   func_write_s16    = lu_BitstreamWrite_s16,  \
   func_write_s32    = lu_BitstreamWrite_s32,  \
   func_write_string = lu_BitstreamWrite_string_optional_terminator, \
   func_write_string_terminated = lu_BitstreamWrite_string, \
)
#pragma lu_bitpack layout( \
   sectors (               \
      count     = 2,       \
      variables = 2        \
   ),                      \
   sectors (               \
      variables = 1        \
   )                       \
)

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
   //
   // The goal is to have this function's code generated entirely by this 
   // pragma. Right now, we only serialize a single sector, but we want 
   // to eventually have an if/else tree on the sector ID and delegate 
   // out to separate per-sector functions whenever multiple sectors are 
   // used. The plug-in should generate those per-sector functions from 
   // scratch.
   //
   #pragma lu_bitpack generate_pack( \
      data   = ( sStructA, sStructB, sStructC ), \
      buffer = dst,      \
      sector = sector_id \
   )
   
   struct lu_BitstreamState state;
   lu_BitstreamInitialize(&state, dst);
   
   for(int i = 0; i < 9; ++i)
      lu_BitstreamWrite_u8(&state, sStructA.a[i], 6);
   
   lu_BitstreamWrite_u8(&state, sStructA.b, 5);
   
   lu_BitstreamWrite_string(&state, sStructB.a, 3);
}
#pragma lu_bitpack debug_dump_function save

extern void read(const u8* src, int sector_id) {
   //
   // The goal is to have this function's code generated entirely by this 
   // pragma. Right now, we only serialize a single sector, but we want 
   // to eventually have an if/else tree on the sector ID and delegate 
   // out to separate per-sector functions whenever multiple sectors are 
   // used. The plug-in should generate those per-sector functions from 
   // scratch.
   //
   #pragma lu_bitpack generate_read( \
      data   = ( sStructA, sStructB, sStructC ), \
      buffer = src,      \
      sector = sector_id \
   )
   
   struct lu_BitstreamState state;
   lu_BitstreamInitialize(&state, (u8*)src);
   
   for(int i = 0; i < 9; ++i)
      sStructA.a[i] = lu_BitstreamRead_u8(&state, 6);
   
   sStructA.b = lu_BitstreamRead_u8(&state, 5);
   
   lu_BitstreamRead_string(&state, sStructB.a, 5);
}
#pragma lu_bitpack debug_dump_function read

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