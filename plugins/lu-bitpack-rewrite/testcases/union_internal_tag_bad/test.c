
//
// Test-case: simple type transformations.
//

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

union LU_BP_UNION_INTERNAL_TAG(tag) TestBitpackOptionsDiverge {
   LU_BP_TAGGED_ID(0) struct {
      LU_BP_BITCOUNT(3) u8  header;
      LU_BP_BITCOUNT(1) u8  tag;
      u16 data;
   } a;
   LU_BP_TAGGED_ID(1) struct {
      LU_BP_BITCOUNT(2) u8  header;
      LU_BP_BITCOUNT(1) u8  tag;
      u32 data;
   } b;
};

union LU_BP_UNION_INTERNAL_TAG(tag) TestNonIntegralTag {
   LU_BP_TAGGED_ID(0) struct {
      u8    header;
      float tag;
      u16   data;
   } a;
   LU_BP_TAGGED_ID(1) struct {
      u8    header;
      float tag;
      u32   data;
   } b;
};

union LU_BP_UNION_INTERNAL_TAG(tag) TestValid {
   LU_BP_TAGGED_ID(0) struct {
      u8  header;
      u8  tag;
      u16 data;
   } a;
   LU_BP_TAGGED_ID(1) struct {
      u8  header;
      u8  tag;
      u32 data;
   } b;
};

static struct TestStruct {
   union TestBitpackOptionsDiverge invalid_diverge;
   union TestNonIntegralTag        invalid_non_integral;
   
   union TestValid valid;
} sTestStruct;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct, \
   enable_debug_output = true \
)
//#pragma lu_bitpack debug_dump_function generated_read
//#pragma lu_bitpack debug_dump_function __lu_bitpack_read_sector_0
//#pragma lu_bitpack debug_dump_function __lu_bitpack_save_TestStruct
