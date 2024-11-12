
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

union LU_BP_UNION_INTERNAL_TAG(tag) InternallyTaggedValid {
   LU_BP_TAGGED_ID(0) struct {
      uint32_t header;
      uint32_t tag;
   } a;
   LU_BP_TAGGED_ID(1) struct {
      uint32_t header;
      uint32_t tag;
      LU_BP_AS_OPAQUE_BUFFER float data;
   } b;
   LU_BP_TAGGED_ID(2) struct {
      uint32_t header;
      uint32_t tag;
      uint32_t data;
   } c;
};

union LU_BP_UNION_INTERNAL_TAG(tag) InternallyTaggedMissingTag {
   LU_BP_TAGGED_ID(0) struct {
      int header;
   } a;
   LU_BP_TAGGED_ID(1) struct {
      int   header;
      float data;
   } b;
   LU_BP_TAGGED_ID(2) struct {
      int header;
      int data;
   } c;
};

union LU_BP_UNION_INTERNAL_TAG(tag) InternallyTaggedLateTag {
   LU_BP_TAGGED_ID(0) struct {
      int header;
      int tag;
   } a;
   LU_BP_TAGGED_ID(1) struct {
      int   header;
      float data;
      int   tag;
   } b;
   LU_BP_TAGGED_ID(2) struct {
      int header;
      int data;
      int tag;
   } c;
};

LU_BP_UNION_INTERNAL_TAG(tag) typedef union {
   LU_BP_TAGGED_ID(0) struct {
      int header;
   } a;
   LU_BP_TAGGED_ID(1) struct {
      int   header;
      float data;
   } b;
   LU_BP_TAGGED_ID(2) struct {
      int header;
      int data;
   } c;
} InternallyTaggedMissingTagTypedef;

struct ExternallyTaggedValid {
   int tag;
   LU_BP_UNION_TAG(tag) union {
      LU_BP_TAGGED_ID(0) int   a;
      LU_BP_TAGGED_ID(1) float b;
   } data;
};

struct ExternallyTaggedMissingTag {
   int not_a_tag;
   LU_BP_UNION_TAG(tag) union {
      LU_BP_TAGGED_ID(0) int   a;
      LU_BP_TAGGED_ID(1) float b;
   } data;
};

struct ExternallyTaggedLateTag {
   LU_BP_UNION_TAG(tag) union {
      LU_BP_TAGGED_ID(0) int   a;
      LU_BP_TAGGED_ID(1) float b;
   } data;
   int tag;
};

struct ExternallyTaggedBadNesting {
   int tag;
   struct {
      LU_BP_UNION_TAG(tag) union {
         LU_BP_TAGGED_ID(0) int   a;
         LU_BP_TAGGED_ID(1) float b;
      } data;
   } foo;
};

static struct TestStruct {
   LU_BP_BITCOUNT(3) u8 a;
   union  InternallyTaggedValid internal;
   struct ExternallyTaggedValid external;
} sTestStruct;
#pragma lu_bitpack debug_dump_as_serialization_item sTestStruct