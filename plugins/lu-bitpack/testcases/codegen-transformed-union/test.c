
#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#define SECTOR_COUNT 3
#define SECTOR_SIZE 12

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

// Testcase: transformed union.
struct PackedUnionA;
union UnionA;
void PackUnionA(const union UnionA*, struct PackedUnionA*);
void UnpackUnionA(union UnionA*, const struct PackedUnionA*);
struct PackedUnionA {
   int tag;
   int data;
};
union LU_BP_TRANSFORM(PackUnionA,UnpackUnionA) UnionA {
   struct {
      int tag;
      int data;
   } x;
   struct {
      int tag;
      int data;
   } y;
};
void PackUnionA(const union UnionA* src, struct PackedUnionA* dst) {
   dst->tag  = src->x.tag;
   dst->data = src->x.data;
}
void UnpackUnionA(union UnionA* dst, const struct PackedUnionA* src) {
   dst->x.tag  = src->tag;
   dst->x.data = src->data;
}

// Containing test struct.
struct TestStruct {
   union UnionA a;
   
   // padding, to force sTestStruct to be split across sectors -- a 
   // cheap hack so that the debug dump lets us see the serialization 
   // items used to represent `sTestStruct.a`.
   int force_expand_sTestStruct[3];
} sTestStruct;


#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct,            \
   enable_debug_output = true \
)

//
// Testing:
//

#include <string.h> // memset

void print_test_struct() {
   printf("sTestStruct == {\n");
   printf("   .a == {\n");
   switch (sTestStruct.a.x.tag) {
      case 0:
         printf("      .x == {\n");
         printf("         .tag  == %d,\n", sTestStruct.a.x.tag);
         printf("         .data == %d,\n", sTestStruct.a.x.data);
         printf("      },\n");
         break;
      case 1:
         printf("      .y == {\n");
         printf("         .tag  == %d,\n", sTestStruct.a.x.tag);
         printf("         .data == %d,\n", sTestStruct.a.x.data);
         printf("      },\n");
         break;
      default:
         printf("      ??? == {\n");
         printf("         .tag  == %d,\n", sTestStruct.a.x.tag);
         printf("         .data == %d,\n", sTestStruct.a.x.data);
         printf("      },\n");
         break;
   }
   printf("   },\n");
   printf("}\n");
}

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sTestStruct.a.x.tag  = 0;
   sTestStruct.a.x.data = 55555;
   
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