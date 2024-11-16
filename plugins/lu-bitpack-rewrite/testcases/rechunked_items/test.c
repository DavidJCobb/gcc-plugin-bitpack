
#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#pragma lu_bitpack enable
#pragma lu_bitpack set_options ( \
   sector_count=9999, \
   sector_size=16,  \
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

struct PackedColor;
struct Color;
void PackColor(const struct Color*, struct PackedColor*);
void UnpackColor(struct Color*, const struct PackedColor*);
//
struct PackedColor {
   int r;
   int g;
};
struct LU_BP_TRANSFORM(PackColor,UnpackColor) Color {
   int r;
   int g;
   int b;
};

struct PackedNestedInner;
struct NestedInner;
void PackNestedInner(const struct NestedInner*, struct PackedNestedInner*);
void UnpackNestedInner(struct NestedInner*, const struct PackedNestedInner*);
//
struct PackedNestedOuter;
struct NestedOuter;
void PackNestedOuter(const struct NestedOuter*, struct PackedNestedOuter*);
void UnpackNestedOuter(struct NestedOuter*, const struct PackedNestedOuter*);
//
struct PackedNestedInner {
   LU_BP_BITCOUNT(2) int a;
   LU_BP_BITCOUNT(2) int b;
   LU_BP_BITCOUNT(2) int c;
   LU_BP_BITCOUNT(2) int tag;
   LU_BP_UNION_TAG(tag) union {
      LU_BP_TAGGED_ID(0) int a;
      LU_BP_TAGGED_ID(1) int b;
      LU_BP_TAGGED_ID(2) int c;
   } data;
};
struct LU_BP_TRANSFORM(PackNestedInner,UnpackNestedInner) NestedInner {};
struct PackedNestedOuter {
   LU_BP_BITCOUNT(3) int a;
   struct NestedInner b[2];
};
struct LU_BP_TRANSFORM(PackNestedOuter,UnpackNestedOuter) NestedOuter {};

struct TransitiveStart;
struct TransitiveMiddle;
struct TransitiveFinal;
//
void PackTransitiveStart(const struct TransitiveStart*, struct TransitiveMiddle*);
void UnpackTransitiveStart(struct TransitiveStart*, const struct TransitiveMiddle*);
//
void PackTransitiveMiddle(const struct TransitiveMiddle*, struct TransitiveFinal*);
void UnpackTransitiveMiddle(struct TransitiveMiddle*, const struct TransitiveFinal*);
//
struct TransitiveFinal {
   int a;
};
struct LU_BP_TRANSFORM(PackTransitiveMiddle,UnpackTransitiveMiddle) TransitiveMiddle {
};
struct LU_BP_TRANSFORM(PackTransitiveStart,UnpackTransitiveStart) TransitiveStart {
};

static struct TestStruct {
   int a[3];
   int b[3];
   struct {
      int tag;
      LU_BP_UNION_TAG(tag) union {
         LU_BP_TAGGED_ID(0) int a;
         LU_BP_TAGGED_ID(1) int b;
         LU_BP_TAGGED_ID(2) int c[2];
      } data;
   } foo[2];
   int x;
   int y;
   struct {
      int tag;
      LU_BP_UNION_TAG(tag) union {
         LU_BP_TAGGED_ID(0) int a;
         LU_BP_TAGGED_ID(1) int b;
         LU_BP_TAGGED_ID(2) int c[2];
         LU_BP_TAGGED_ID(3) struct {
            int tag;
            LU_BP_UNION_TAG(tag) union {
               LU_BP_TAGGED_ID(0) int a;
               LU_BP_TAGGED_ID(1) int b;
            } data;
         } d;
      } data;
   } z;

   int single_array_element[1];
   int single_array_element_2D[1][1];
   int single_array_element_3D[1][1][1];

   struct Color color_a;
   struct NestedOuter nested_trans;
   struct TransitiveStart transitive_trans;
} sTestStruct;

#pragma lu_bitpack debug_dump_as_serialization_item sTestStruct

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)
