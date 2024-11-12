
#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#pragma lu_bitpack enable
#pragma lu_bitpack set_options ( \
   sector_count=9, \
   sector_size=8,  \
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

struct Color;
struct PackedColor;
void PackColor(const struct Color*, struct PackedColor*);
void UnpackColor(struct Color*, const struct PackedColor*);

struct PackedColor {
   LU_BP_BITCOUNT(3) u8 r;
   LU_BP_BITCOUNT(2) u8 g;
   LU_BP_BITCOUNT(2) u8 b;
};

struct LU_BP_TRANSFORM(PackColor,UnpackColor) Color {
   u8 r;
   u8 g;
   u8 b;
};

//union LU_BP_BITCOUNT(3) Test {
//};
//LU_BP_BITCOUNT(3) typedef union {} Test2;

static struct TestStruct {
   bool8 a;
   LU_BP_BITCOUNT(3) u8 b;
   struct {
      u8 c_x;
      u8 c_y;
   } c;
   u8 d[5];
   LU_BP_OMIT float e;
   struct Color f;
   u8 g[4][3][2];
   LU_BP_OMIT LU_BP_STRING_UT LU_BP_DEFAULT("Lucia") char name[5];
} sTestStruct;

#pragma lu_bitpack debug_dump_as_serialization_item sTestStruct

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)
