
//
// Test-case: nested type transformations.
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

struct Inner;
struct PackedInner;
void PackInner(const struct Inner*, struct PackedInner*);
void UnpackInner(struct Inner*, const struct PackedInner*);

struct Outer;
struct PackedOuter;
void PackOuter(const struct Outer*, struct PackedOuter*);
void UnpackOuter(struct Outer*, const struct PackedOuter*);

struct LU_BP_TRANSFORM(PackInner,UnpackInner) Inner {
   u32 n;
};
struct PackedInner {
   u16 n;
};

struct LU_BP_TRANSFORM(PackOuter,UnpackOuter) Outer {
   u32 a;
   u32 b;
};
struct PackedOuter {
   u8 a;
   struct PackedInner b;
};

void PackInner(const struct Inner* src, struct PackedInner* dst) {
   dst->n = src->n;
}
void UnpackInner(struct Inner* dst, const struct PackedInner* src) {
   dst->n = src->n;
}

void PackOuter(const struct Outer* src, struct PackedOuter* dst) {
   dst->a = src->a;
   dst->b.n = src->b;
}
void UnpackOuter(struct Outer* dst, const struct PackedOuter* src) {
   dst->a = src->a;
   dst->b = src->b.n;
}



static struct Outer sOuter;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sOuter,                 \
   enable_debug_output = true \
)
//#pragma lu_bitpack debug_dump_function generated_read
//#pragma lu_bitpack debug_dump_function __lu_bitpack_read_sector_0
//#pragma lu_bitpack debug_dump_function __lu_bitpack_save_TestStruct

#include <string.h> // memset

void print_test_data() {
   printf("sOuter == {\n");
   printf("   .a = %u\n", sOuter.a);
   printf("   .b = %u\n", sOuter.b);
   printf("}\n");
}

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sOuter.a = 55;
   sOuter.b = 66;
   
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
   memset(&sOuter, 0xCC, sizeof(sOuter));
   
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      printf("Sector %u saved:\n", i);
      print_buffer(sector_buffers[i], sizeof(sector_buffers[i]));
      printf("Sector %u read:\n", i);
      generated_read(sector_buffers[i], i);
      print_test_data();
   }
   
   return 0;
}