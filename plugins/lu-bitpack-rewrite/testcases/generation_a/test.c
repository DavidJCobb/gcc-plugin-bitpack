
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

#define TEST_ARRAYS 1
#define TEST_UNIONS 1

static struct TestStruct {
   #if TEST_ARRAYS
   u32 a[3];
   u32 b[3];
   #endif
   #if TEST_UNIONS && TEST_ARRAYS
   struct {
      u32 tag;
      LU_BP_UNION_TAG(tag) union {
         LU_BP_TAGGED_ID(0) u32 a;
         LU_BP_TAGGED_ID(1) u32 b;
         LU_BP_TAGGED_ID(2) u32 c[2];
      } data;
   } foo[2];
   #endif
   u32 x;
   u32 y;
   #if TEST_UNIONS
   struct {
      u32 tag;
      LU_BP_UNION_TAG(tag) union {
         LU_BP_TAGGED_ID(0) u32 a;
         LU_BP_TAGGED_ID(1) u32 b;
         #if TEST_ARRAYS
         LU_BP_TAGGED_ID(2) u32 c[2];
         #endif
         LU_BP_TAGGED_ID(3) struct {
            u32 tag;
            LU_BP_UNION_TAG(tag) union {
               LU_BP_TAGGED_ID(0) u32 a;
               LU_BP_TAGGED_ID(1) u32 b;
            } data;
         } d;
      } data;
   } z;
   #endif

   #if TEST_ARRAYS
   u32 single_array_element[1];
   u32 single_array_element_2D[1][1];
   u32 single_array_element_3D[1][1][1];
   #endif
} sTestStruct;
#pragma lu_bitpack debug_dump_as_serialization_item sTestStruct

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)
#pragma lu_bitpack debug_dump_function __lu_bitpack_save_sector_0

#include <string.h> // memset

void print_test_struct() {
   printf("sTestStruct == {\n");
   #if TEST_ARRAYS
   printf("   .a == { %d, %d, %d },\n", sTestStruct.a[0], sTestStruct.a[1], sTestStruct.a[2]);
   printf("   .b == { %d, %d, %d },\n", sTestStruct.b[0], sTestStruct.b[1], sTestStruct.b[2]);
   #endif
   #if TEST_UNIONS && TEST_ARRAYS
   printf("   .foo = {\n");
   for(int i = 0; i < 2; ++i) {
      printf("      {\n");
      printf("         .tag = %d,\n", sTestStruct.foo[i].tag);
      printf("         .data = {\n");
      switch (sTestStruct.foo[i].tag) {
         case 0:
            printf("            .a = %d,\n", sTestStruct.foo[i].data.a);
            break;
         case 1:
            printf("            .b = %d,\n", sTestStruct.foo[i].data.b);
            break;
         case 2:
            printf("            .c = { %d, %d },\n", sTestStruct.foo[i].data.c[0], sTestStruct.foo[i].data.c[1]);
            break;
      }
      printf("         },\n");
      printf("      },\n");
   }
   printf("   },\n");
   #endif
   printf("   .x == %d,\n", sTestStruct.x);
   printf("   .y == %d,\n", sTestStruct.y);
   #if TEST_UNIONS
   printf("   .z = {\n");
   printf("      .tag = %d,\n", sTestStruct.z.tag);
   printf("      .data = {\n");
   switch (sTestStruct.z.tag) {
      case 0:
         printf("         .a = %d,\n", sTestStruct.z.data.a);
         break;
      case 1:
         printf("         .b = %d,\n", sTestStruct.z.data.b);
         break;
      #if TEST_ARRAYS
      case 2:
         printf("         .c = { %d, %d },\n", sTestStruct.z.data.c[0], sTestStruct.z.data.c[1]);
         break;
      #endif
      case 3:
         printf("         .d = {\n");
         printf("            .tag = %d,\n", sTestStruct.z.data.d.tag);
         printf("            .data = {\n");
         switch (sTestStruct.z.data.d.tag) {
            case 0:
               printf("               .a = %d,\n", sTestStruct.z.data.d.data.a);
               break;
            case 1:
               printf("               .b = %d,\n", sTestStruct.z.data.d.data.b);
               break;
         }
         printf("            },\n");
         printf("         },\n");
         break;
   }
   printf("      },\n");
   printf("   },\n");
   #endif
   #if TEST_ARRAYS
   printf("   .single_array_element == { %d },\n", sTestStruct.single_array_element[0]);
   printf("   .single_array_element_2D == { { %d } },\n", sTestStruct.single_array_element_2D[0][0]);
   printf("   .single_array_element_3D == { { { %d } } },\n", sTestStruct.single_array_element_3D[0][0][0]);
   printf("}\n");
   #endif
}

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   #if TEST_ARRAYS
   for(int i = 0; i < 3; ++i) {
      sTestStruct.a[i] = i + 1;
      sTestStruct.b[i] = i + 3 + 1;
   }
   #endif
   #if TEST_ARRAYS && TEST_UNIONS
   sTestStruct.foo[0].tag = 0;
   sTestStruct.foo[0].data.a = 55;
   sTestStruct.foo[1].tag = 2;
   sTestStruct.foo[1].data.c[0] = 66;
   sTestStruct.foo[1].data.c[1] = 77;
   #endif
   sTestStruct.x = 12;
   sTestStruct.y = 34;
   #if TEST_UNIONS
   sTestStruct.z.tag = 3;
   sTestStruct.z.data.d.tag = 0;
   sTestStruct.z.data.d.data.a = 77;
   #endif
   #if TEST_ARRAYS
   sTestStruct.single_array_element[0] = 1;
   sTestStruct.single_array_element_2D[0][0] = 2;
   sTestStruct.single_array_element_3D[0][0][0] = 3;
   #endif
   
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