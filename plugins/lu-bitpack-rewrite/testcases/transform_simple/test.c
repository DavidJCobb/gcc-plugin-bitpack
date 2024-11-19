
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

struct Color;
struct PackedColor;
void PackColor(const struct Color*, struct PackedColor*);
void UnpackColor(struct Color*, const struct PackedColor*);

struct LU_BP_TRANSFORM(PackColor,UnpackColor) Color {
   u16 r;
   u16 g;
   u16 b;
};
struct PackedColor {
   u8 r;
   u8 g;
   u8 b;
};
void PackColor(const struct Color* src, struct PackedColor* dst) {
   dst->r = src->r;
   dst->g = src->g;
   dst->b = src->b;
}
void UnpackColor(struct Color* dst, const struct PackedColor* src) {
   dst->r = src->r;
   dst->g = src->g;
   dst->b = src->b;
}

static struct Color sTestColor;
static struct TestStruct {
   u8           padding[SECTOR_SIZE - 3 - 2]; // large enough to force next onto a sector boundary
   struct Color second_color;
} sTestStruct;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestColor sTestStruct, \
   enable_debug_output = true \
)
//#pragma lu_bitpack debug_dump_function generated_read
//#pragma lu_bitpack debug_dump_function __lu_bitpack_read_sector_0
//#pragma lu_bitpack debug_dump_function __lu_bitpack_save_TestStruct

#include <string.h> // memset

void print_test_data() {
   printf("sTestColor == {\n");
   printf("   .r = %u\n", sTestColor.r);
   printf("   .g = %u\n", sTestColor.g);
   printf("   .b = %u\n", sTestColor.b);
   printf("}\n");
   printf("sTestStruct == {\n");
   printf("   .padding = {\n");
   for(int i = 0; i < sizeof(sTestStruct.padding); ++i)
      printf("      %02X,\n", sTestStruct.padding[i]);
   printf("   },\n");
   printf("   .second_color = {\n");
   printf("      .r = %u\n", sTestStruct.second_color.r);
   printf("      .g = %u\n", sTestStruct.second_color.g);
   printf("      .b = %u\n", sTestStruct.second_color.b);
   printf("   },\n");
   printf("}\n");
}

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sTestColor.r = 0xFF;
   sTestColor.g = 0x74;
   sTestColor.b = 0x00;
   memset(&sTestStruct, 0x00, sizeof(sTestStruct));
   sTestStruct.second_color = sTestColor;
   
   const char* divider = "====================================================\n";
   
   printf(divider);
   printf("Test data:\n");
   printf(divider);
   print_test_data();
   printf("\n");
   
   //
   // Perform save.
   //
   memset(&sector_buffers, '~', sizeof(sector_buffers));
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      generated_save(sector_buffers[i], i);
   }
   memset(&sTestColor, 0xCC, sizeof(sTestColor));
   memset(&sTestStruct, 0xCC, sizeof(sTestStruct));
   
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      printf("Sector %u saved:\n", i);
      print_buffer(sector_buffers[i], sizeof(sector_buffers[i]));
      printf("Sector %u read:\n", i);
      generated_read(sector_buffers[i], i);
      print_test_data();
   }
   
   return 0;
}