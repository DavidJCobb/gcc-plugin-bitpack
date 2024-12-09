
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

LU_BP_BITCOUNT(5) typedef u8 u5;
typedef u5 ufive;

LU_BP_STRING_UT typedef char player_name [7];
typedef player_name character_name;

struct LU_BP_AS_OPAQUE_BUFFER buf {
   u8 data[2];
};
typedef struct buf buf2;
typedef buf2 buftwo;

LU_BP_BITCOUNT(5) typedef u8 u5pair [2];
typedef u5pair two_u5s;

struct TestStruct {
   u5    a;
   ufive a_transitive;
   ufive a_transitive_array;
   
   player_name    b;
   character_name b_transitive;
   
   struct buf c;
   buf2       c_transitive;
   buftwo     c_transitive_again;
   
   u5pair  d; // should be 5 bits
   two_u5s d_transitive; // should also be 5 bits
   two_u5s d_transitive_array; // should be 5 bits per
   LU_BP_BITCOUNT(7) two_u5s d_transitive_array_overridden; // should be 7 bits per
} sTestStruct;

#pragma lu_bitpack debug_dump_bp_data_options TestStruct::a
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::a_transitive
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::a_transitive_array
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::b
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::b_transitive
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::c
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::c_transitive
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::c_transitive_again
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::d
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::d_transitive
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::d_transitive_array
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::d_transitive_array_overridden
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::d_control