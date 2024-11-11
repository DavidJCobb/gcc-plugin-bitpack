
/*

   TESTCASE: Bad transform options

*/

#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

// tests
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

typedef u16 in_situ_type;
typedef u8  transformed_type;

typedef u32 something_else;

void TransformPrePack(const in_situ_type*, transformed_type*);
void TransformPostUnpack(in_situ_type*, const transformed_type*);

void BadTransformPrePack(const something_else*, transformed_type*);
void BadTransformPostUnpack(something_else*, const transformed_type*);

LU_BP_TRANSFORM(TransformPrePack,TransformPostUnpack) typedef in_situ_type in_situ_type_alt;

static struct TestStruct {
   LU_BP_TRANSFORM(TransformPrePack,TransformPostUnpack) in_situ_type a;
   LU_BP_TRANSFORM(TransformPrePack,TransformPostUnpack) in_situ_type b : 5;
   in_situ_type_alt c;
   in_situ_type_alt d : 5;
   
   LU_BP_TRANSFORM(TransformPrePack,BadTransformPostUnpack) in_situ_type e;
   LU_BP_TRANSFORM(BadTransformPrePack,BadTransformPostUnpack) in_situ_type f;
} sTestStruct;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)
