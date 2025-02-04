
#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#define SECTOR_COUNT 7
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

// Testcase: transform options on a RECORD_TYPE.
struct PackedColor;
struct Color;
void PackColor(const struct Color*, struct PackedColor*);
void UnpackColor(struct Color*, const struct PackedColor*);

struct PackedColor {
   LU_BP_BITCOUNT(24) unsigned int tuple;
};
struct LU_BP_TRANSFORM(PackColor,UnpackColor) Color {
   u8 r;
   u8 g;
   u8 b;
};

// Testcase: transform options on a FIELD_DECL.
struct PackedBespoke;
struct Bespoke;
void PackBespoke(const struct Bespoke*, struct PackedBespoke*);
void UnpackBespoke(struct Bespoke*, const struct PackedBespoke*);

struct PackedBespoke {
   LU_BP_BITCOUNT(3) u8 a;
};
struct Bespoke {
   int a;
};

struct TestStruct {
   struct Color a;
   LU_BP_TRANSFORM(PackBespoke,UnpackBespoke) struct Bespoke b;
};

#pragma lu_bitpack debug_dump_bp_data_options TestStruct
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::a
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::b

//
// Test error cases.
//

// Testcase: not a pack function or not an unpack function.
struct PackedWTF {};
struct WTF;
void PackWTF(int a);
void UnpackWTF(struct PackedWTF*, float);
//
struct LU_BP_TRANSFORM(PackWTF, UnpackWTF) WTF {};

// Testcase: incorrect const qualifiers on the (un)pack function arguments.
struct PackedWrongConst {};
struct WrongConst;
void PackWrongConst(struct WrongConst*, const struct PackedWrongConst*);
void UnpackWrongConst(const struct WrongConst*, struct PackedWrongConst*);
//
struct LU_BP_TRANSFORM(PackWrongConst, UnpackWrongConst) WrongConst {};

// Testcase: mismatched types on the (un)pack function arguments.
struct PackedMismatchedTypeA {};
struct PackedMismatchedTypeB {};
struct MismatchedType;
void PackMismatchedType(const struct MismatchedType*, struct PackedMismatchedTypeA*);
void UnpackMismatchedType(struct MismatchedType*, const struct PackedMismatchedTypeB*);
//
struct LU_BP_TRANSFORM(PackMismatchedType, UnpackMismatchedType) MismatchedType {};

struct BadStruct {
   struct WrongConst     a;
   struct MismatchedType b;
   struct WTF            c;
};