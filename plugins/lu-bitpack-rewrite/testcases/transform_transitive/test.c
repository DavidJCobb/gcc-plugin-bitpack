
//
// Test-case: transitive type transformations.
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

struct TypeA;
struct TypeB;
struct TypeC;
void PackTypeA(const struct TypeA*, struct TypeB*);
void UnpackTypeA(struct TypeA*, const struct TypeB*);
void PackTypeB(const struct TypeB*, struct TypeC*);
void UnpackTypeB(struct TypeB*, const struct TypeC*);

struct LU_BP_TRANSFORM(PackTypeA,UnpackTypeA) TypeA {
   u32 n;
};
struct LU_BP_TRANSFORM(PackTypeB,UnpackTypeB) TypeB {
   u16 n;
};
struct TypeC {
   u8 n;
};

void PackTypeA(const struct TypeA* src, struct TypeB* dst) {
   dst->n = src->n;
}
void UnpackTypeA(struct TypeA* dst, const struct TypeB* src) {
   dst->n = src->n;
}
void PackTypeB(const struct TypeB* src, struct TypeC* dst) {
   dst->n = src->n;
}
void UnpackTypeB(struct TypeB* dst, const struct TypeC* src) {
   dst->n = src->n;
}

static struct TypeA sTypeA;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTypeA,                 \
   enable_debug_output = true \
)
//#pragma lu_bitpack debug_dump_function generated_read
//#pragma lu_bitpack debug_dump_function __lu_bitpack_read_sector_0
//#pragma lu_bitpack debug_dump_function __lu_bitpack_save_TestStruct

#include <string.h> // memset

void print_test_data() {
   printf("sTypeA == {\n");
   printf("   .n = %u\n", sTypeA.n);
   printf("}\n");
}

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sTypeA.n = 55;
   
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
   memset(&sTypeA, 0xCC, sizeof(sTypeA));
   
   for(int i = 0; i < SECTOR_COUNT; ++i) {
      printf("Sector %u saved:\n", i);
      print_buffer(sector_buffers[i], sizeof(sector_buffers[i]));
      printf("Sector %u read:\n", i);
      generated_read(sector_buffers[i], i);
      print_test_data();
   }
   
   return 0;
}