
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

struct TestStruct {
   int a_tag;
   /*LU_BP_UNION_TAG(a_tag)*/ __attribute__((lu_bitpack_union_external_tag("a_tag"))) union {
      LU_BP_TAGGED_ID(0) int x;
      LU_BP_TAGGED_ID(1) int y;
      LU_BP_TAGGED_ID(2) int z;
   } a_data;
   
   LU_BP_UNION_INTERNAL_TAG(tag) union {
      LU_BP_TAGGED_ID(0) struct {
         int header;
         int tag;
      } x;
      LU_BP_TAGGED_ID(1) struct {
         int header;
         int tag;
         int data;
      } y;
      LU_BP_TAGGED_ID(2) struct {
         int  header;
         int  tag;
         char data;
      } z;
   } b;
   
   // actually, maybe this SHOULD fail. C doesn't mandate 
   // that `int` be 32 bits, after all.
   LU_BP_UNION_INTERNAL_TAG(tag) union {
      LU_BP_TAGGED_ID(0) struct {
         int tag;
      } x;
      LU_BP_TAGGED_ID(1) struct {
         LU_BP_BITCOUNT(sizeof(int) * 8) int tag;
      } y;
   } different_options_but_equal_in_effect;
};

struct BadStruct {
   int a_tag;
   struct {
      LU_BP_UNION_TAG(a_tag) union {
      } a_data;
   } a_wrapped;
   
   LU_BP_UNION_TAG(nope) union {
   } missing_tag;
   
   LU_BP_UNION_TAG(a_tag) union {
      int a;
   } externally_tagged_with_member_sans_id;
   
   LU_BP_UNION_INTERNAL_TAG(tag) union {
      struct {
         int tag;
      } a;
   } internally_tagged_with_member_sans_id;
   
   LU_BP_UNION_INTERNAL_TAG(tag) union {
      LU_BP_TAGGED_ID(0) struct {
         int header;
         int tag;
      } x;
      LU_BP_TAGGED_ID(1) struct {
         int tag;
         int header;
      } y;
   } internal_tag_out_of_order;
   
   LU_BP_UNION_INTERNAL_TAG(tag) union {
      LU_BP_TAGGED_ID(0) struct {
         int header;
         int tag;
      } x;
      LU_BP_TAGGED_ID(1) struct {
         int  header;
         char tag;
      } y;
   } internal_tag_different_types;
   
   LU_BP_UNION_INTERNAL_TAG(tag) union {
      LU_BP_TAGGED_ID(0) struct {
         int header;
         int tag;
      } x;
      LU_BP_TAGGED_ID(1) struct {
         int header;
         LU_BP_BITCOUNT(7) int tag;
      } y;
   } internal_tag_different_options;
   
   LU_BP_UNION_TAG(a_tag) LU_BP_UNION_INTERNAL_TAG(tag) union {
   } contradictorily_tagged_union;
   
   LU_BP_UNION_INTERNAL_TAG(tag) union {
   } empty_internally_tagged_union;
};
   
union LU_BP_UNION_INTERNAL_TAG(tag) MisorderedInternalTag {
   LU_BP_TAGGED_ID(0) struct {
      int header;
      int tag;
   } x;
   LU_BP_TAGGED_ID(1) struct {
      int tag;
      int header;
   } y;
};