
#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#define SECTOR_COUNT 4
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

union LU_BP_UNION_INTERNAL_TAG(tag) InternalTagType {
   LU_BP_TAGGED_ID(0) struct {
      int header;
      int tag;
   } a;
   LU_BP_TAGGED_ID(1) struct {
      int header;
      int tag;
      int data;
   } b;
};

struct TestStruct {
   int external_tag;
   LU_BP_UNION_TAG(external_tag) union {
      LU_BP_TAGGED_ID(0) LU_BP_BITCOUNT(31) int x;
      LU_BP_TAGGED_ID(1) LU_BP_BITCOUNT(15) int y;
      LU_BP_TAGGED_ID(2) LU_BP_BITCOUNT(7) int z;
   } external_data;
   
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
   } internal;
   union InternalTagType typed;
   
   int nested_outer_tag;
   LU_BP_UNION_TAG(nested_outer_tag) union {
      LU_BP_TAGGED_ID(0) LU_BP_UNION_INTERNAL_TAG(nested_inner_tag) union {
         LU_BP_TAGGED_ID(0) struct {
            int nested_inner_tag;
         } a;
         LU_BP_TAGGED_ID(1) struct {
            int nested_inner_tag;
            int data;
         } b;
      } inner;
   } outer;
} sTestStruct;
 
#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct,            \
   enable_debug_output = true \
)
//#pragma lu_bitpack debug_dump_function __lu_bitpack_read_sector_0

//
// Testing:
//

#include <string.h> // memset

void print_test_struct() {
   printf("sTestStruct == {\n");
   printf("   .external_tag == %d\n", sTestStruct.external_tag);
   printf("   .external_data == {\n");
   switch (sTestStruct.external_tag) {
      case 0:
         printf("      .x == %u\n", sTestStruct.external_data.x);
         break;
      case 1:
         printf("      .x == %u\n", sTestStruct.external_data.y);
         break;
      case 2:
         printf("      .x == %u\n", sTestStruct.external_data.z);
         break;
      default:
         printf("      ?\n");
   }
   printf("   },\n");
   printf("   .internal == {\n");
   switch (sTestStruct.internal.x.tag) {
      case 0:
         printf("      .x == {\n");
         printf("         .header == %u,\n", sTestStruct.internal.x.header);
         printf("         .tag    == %u,\n", sTestStruct.internal.x.tag);
         printf("      },\n");
         break;
      case 1:
         printf("      .y == {\n");
         printf("         .header == %u,\n", sTestStruct.internal.y.header);
         printf("         .tag    == %u,\n", sTestStruct.internal.y.tag);
         printf("         .data   == %u,\n", sTestStruct.internal.y.data);
         printf("      },\n");
         break;
      case 2:
         printf("      .z == {\n");
         printf("         .header == %u,\n", sTestStruct.internal.z.header);
         printf("         .tag    == %u,\n", sTestStruct.internal.z.tag);
         printf("         .data   == %u,\n", sTestStruct.internal.z.data);
         printf("      },\n");
         break;
      default:
         printf("      ?\n");
   }
   printf("   },\n");
   printf("   .typed == {\n");
   switch (sTestStruct.typed.a.tag) {
      case 0:
         printf("      .a == {\n");
         printf("         .header == %u,\n", sTestStruct.typed.a.header);
         printf("         .tag    == %u,\n", sTestStruct.typed.a.tag);
         printf("      },\n");
         break;
      case 1:
         printf("      .b == {\n");
         printf("         .header == %u,\n", sTestStruct.typed.b.header);
         printf("         .tag    == %u,\n", sTestStruct.typed.b.tag);
         printf("         .data   == %u,\n", sTestStruct.typed.b.data);
         printf("      },\n");
         break;
      default:
         printf("      ?\n");
   }
   printf("   },\n");
   printf("   .nested_outer_tag == %d\n", sTestStruct.nested_outer_tag);
   printf("   .outer == {\n");
   switch (sTestStruct.nested_outer_tag) {
      case 0:
         printf("      .inner == {\n");
         switch (sTestStruct.outer.inner.a.nested_inner_tag) {
            case 0:
               printf("         .a == {\n");
               printf("            .nested_inner_tag == %d,\n", sTestStruct.outer.inner.a.nested_inner_tag);
               printf("         }\n");
               break;
            case 1:
               printf("         .b == {\n");
               printf("            .nested_inner_tag == %d,\n", sTestStruct.outer.inner.b.nested_inner_tag);
               printf("            .data == %d,\n", sTestStruct.outer.inner.b.data);
               printf("         }\n");
               break;
            default:
               printf("         ?\n");
         }
         printf("      },\n");
         break;
      default:
         printf("      ?\n");
   }
   printf("   },\n");
   printf("}\n");
}

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sTestStruct.external_tag = 1;
   sTestStruct.external_data.y = 0x7FFF;
   sTestStruct.internal.x.tag = 1;
   sTestStruct.internal.x.header = 5550555;
   sTestStruct.internal.y.data = 123;
   sTestStruct.typed.a.header = 1230123;
   sTestStruct.typed.a.tag = 1;
   sTestStruct.typed.b.data = 55;
   sTestStruct.nested_outer_tag = 0;
   sTestStruct.outer.inner.a.nested_inner_tag = 1;
   sTestStruct.outer.inner.b.data = 123555;
   
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