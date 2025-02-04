
#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#define SECTOR_COUNT 5
#define SECTOR_SIZE 7

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

void PackColor(const struct Color* src, struct PackedColor* dst) {
   dst->tuple = (src->r << 16) | (src->g << 8) | src->b;
}
void UnpackColor(struct Color* dst, const struct PackedColor* src) {
   dst->r = src->tuple >> 16;
   dst->g = src->tuple >> 8;
   dst->b = src->tuple;
}

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

void PackBespoke(const struct Bespoke* src, struct PackedBespoke* dst) {
   dst->a = src->a;
}
void UnpackBespoke(struct Bespoke* dst, const struct PackedBespoke* src) {
   dst->a = src->a;
}

// Testcase: transitive transforms.
struct TransitiveA;
struct TransitiveB;
struct TransitiveC;
void PackTransitiveA(const struct TransitiveA*, struct TransitiveB*);
void UnpackTransitiveA(struct TransitiveA*, const struct TransitiveB*);
void PackTransitiveB(const struct TransitiveB*, struct TransitiveC*);
void UnpackTransitiveB(struct TransitiveB*, const struct TransitiveC*);
struct LU_BP_TRANSFORM(PackTransitiveA,UnpackTransitiveA) TransitiveA {
   u16 a;
   u16 b;
};
struct LU_BP_TRANSFORM(PackTransitiveB,UnpackTransitiveB) TransitiveB {
   u32 merged;
};
struct TransitiveC {
   u16 y;
   u16 x;
};
void PackTransitiveA(const struct TransitiveA* src, struct TransitiveB* dst) {
   dst->merged = ((u32)src->b << 16) | src->a;
}
void UnpackTransitiveA(struct TransitiveA* dst, const struct TransitiveB* src) {
   dst->a = src->merged;
   dst->b = src->merged >> 16;
}
void PackTransitiveB(const struct TransitiveB* src, struct TransitiveC* dst) {
   dst->x = src->merged;
   dst->y = src->merged >> 16;
}
void UnpackTransitiveB(struct TransitiveB* dst, const struct TransitiveC* src) {
   dst->merged = ((u32)src->y << 16) | src->x;
}

// Testcase: nested transforms.
struct NestedOuter;
struct NestedInner;
struct PackedNestedOuter;
struct PackedNestedInner;
void PackNestedOuter(const struct NestedOuter*, struct PackedNestedOuter*);
void UnpackNestedOuter(struct NestedOuter*, const struct PackedNestedOuter*);
void PackNestedInner(const struct NestedInner*, struct PackedNestedInner*);
void UnpackNestedInner(struct NestedInner*, const struct PackedNestedInner*);
struct NestedOuter {
   int a;
   int b;
};
struct NestedInner {
   int data;
};
struct PackedNestedInner {
   LU_BP_BITCOUNT(7) int data;
};
struct PackedNestedOuter {
   LU_BP_BITCOUNT(7) int a;
   struct PackedNestedInner b;
};
void PackNestedOuter(const struct NestedOuter* src, struct PackedNestedOuter* dst) {
   dst->a = src->a;
   dst->b.data = src->b;
}
void UnpackNestedOuter(struct NestedOuter* dst, const struct PackedNestedOuter* src) {
   dst->a = src->a;
   dst->b = src->b.data;
}
void PackNestedInner(const struct NestedInner* src, struct PackedNestedInner* dst) {
   dst->data = src->data;
}
void UnpackNestedInner(struct NestedInner* dst, const struct PackedNestedInner* src) {
   dst->data = src->data;
}


// Containing test struct.
struct TestStruct {
   struct Color a;
   LU_BP_TRANSFORM(PackBespoke,UnpackBespoke) struct Bespoke b;
   struct TransitiveA c;
   struct NestedOuter d;
   
   __attribute__((lu_bitpack_transforms("pre_pack=PackTransitiveA,post_unpack=UnpackTransitiveA,never_split_across_sectors")))
   struct TransitiveA e;
} sTestStruct;


#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct,            \
   enable_debug_output = true \
)
#pragma lu_bitpack debug_dump_bp_data_options TestStruct::e

//
// Testing:
//

#include <string.h> // memset

void print_test_struct() {
   printf("sTestStruct == {\n");
   printf("   .a == {\n");
   printf("      .r == %u\n", sTestStruct.a.r);
   printf("      .g == %u\n", sTestStruct.a.g);
   printf("      .b == %u\n", sTestStruct.a.b);
   printf("   },\n");
   printf("   .b == {\n");
   printf("      .a == %d\n", sTestStruct.b.a);
   printf("   },\n");
   printf("   .c == {\n");
   printf("      .a == %u\n", sTestStruct.c.a);
   printf("      .b == %u\n", sTestStruct.c.b);
   printf("   },\n");
   printf("   .d == {\n");
   printf("      .a == %u\n", sTestStruct.d.a);
   printf("      .b == %u\n", sTestStruct.d.b);
   printf("   },\n");
   printf("   .e == {\n");
   printf("      .a == %d\n", sTestStruct.e.a);
   printf("      .b == %d\n", sTestStruct.e.b);
   printf("   },\n");
   printf("}\n");
}

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sTestStruct.a.r = 0xFF;
   sTestStruct.a.g = 0x74;
   sTestStruct.a.b = 0x00;
   sTestStruct.b.a = 3;
   sTestStruct.c.a = 3;
   sTestStruct.c.b = 3;
   sTestStruct.d.a = 127;
   sTestStruct.d.b = 96;
   sTestStruct.e.a = 3;
   sTestStruct.e.b = 3;
   
   const char* divider = "====================================================\n";
   
   printf(divider);
   printf("Test data:\n");
   printf(divider);
   print_test_struct();
   printf("\n");
   
   //
   // Perform save.
   //
   memset(&sector_buffers, '~', sizeof(sector_buffers));
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      printf("Saving sector %u...\n", i);
      generated_save(sector_buffers[i], i);
   }
   printf("All sectors saved. Wiping sTestStruct...\n");
   memset(&sTestStruct, 0xCC, sizeof(sTestStruct));
   printf("Wiped. Reviewing data...\n\n");
   
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      printf("Sector %u saved:\n", i);
      print_buffer(sector_buffers[i], sizeof(sector_buffers[i]));
      printf("Sector %u read:\n", i);
      generated_read(sector_buffers[i], i);
      print_test_struct();
   }
   
   
   return 0;
}