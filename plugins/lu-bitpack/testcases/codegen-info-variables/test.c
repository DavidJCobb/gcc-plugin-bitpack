
#include "types.h"
#include "bitstreams.h"
#include "helpers.h"

#define SECTOR_COUNT 8
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

// Test nested named structs.
struct Color {
   u8 r;
   u8 g;
   u8 b;
};

// Test bitpacking options on typedefs.
LU_BP_BITCOUNT(5) typedef u8 u5;

// Test stat-tracking categories and bitpacking options on typedefs.
LU_BP_CATEGORY("player-name") LU_BP_STRING_UT typedef u8 player_name [7];

typedef struct {
   u8 data;
} NamedByTypedefOnly;
typedef struct NamedByTag {
   u8 data;
} NamedByTypedef;

struct TestStruct {
   struct Color color;
   
   LU_BP_BITCOUNT(3) u8 three_bit;
   u5 five_bit;
   LU_BP_BITCOUNT(7) u8 seven_bit;
   
   struct {
      // Test multiple stat-tracking categories on a single DECL.
      LU_BP_CATEGORY("327") LU_BP_CATEGORY("second-cat") LU_BP_MINMAX(3, 7) int three_to_seven;
   };
   
   // Test stat-tracking categories on array DECLs.
   LU_BP_CATEGORY("test-names") player_name names[2];
   
   // Vary the test: make `player-name` and `test-names` stats vary from each other.
   LU_BP_DEFAULT("Carter") player_name foe_name;
   
   u8 single_element_array_1D[1];
   u8 single_element_array_2D[1][1];
   u8 single_element_array_3D[1][1][1];
   
   u5 array[3];
   
   bool8 boolean;
   
   LU_BP_AS_OPAQUE_BUFFER float decimals[3];
   
   struct {
      LU_BP_OMIT LU_BP_DEFAULT("Ana")     player_name a;
      LU_BP_OMIT LU_BP_DEFAULT("Kris")    player_name b;
      LU_BP_OMIT LU_BP_DEFAULT("Kara")    player_name c;
      LU_BP_OMIT LU_BP_DEFAULT("Hilda")   player_name d;
      LU_BP_OMIT LU_BP_DEFAULT("Janine")  player_name e;
      LU_BP_OMIT LU_BP_DEFAULT("Darlene") player_name f;
      // Test to verify that the previous name doesn't overflow:
      LU_BP_OMIT LU_BP_DEFAULT("Mary")    player_name g;
   } default_names;
   
   LU_BP_OMIT LU_BP_DEFAULT("<\"xml\">\n<>") LU_BP_STRING char xml_tricky_test[12];
   
   // Testcase: default value is string; value is not marked as a string.
   LU_BP_OMIT LU_BP_DEFAULT("<\"xml\">\n<>") char xml_tricky_test_2[12];
   
   NamedByTypedefOnly named_by_typedef_only;
   struct NamedByTag  using_tag_name;
   NamedByTypedef     using_typedef_name;
} sTestStruct;

struct TestStructAlt {
   LU_BP_BITCOUNT(6) u8 array_a[19];
   LU_BP_BITCOUNT(6) u8 array_b[22];
};
static struct TestStructAlt* sTestStructAltPtr;

struct TestStructWhole {
   LU_BP_BITCOUNT(6) u8 array_a[19];
};
static struct TestStructWhole* sTestStructWholePtr;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct | *sTestStructAltPtr | *sTestStructWholePtr \
   ,enable_debug_output=true\
)
//#pragma lu_bitpack debug_dump_function generated_read

#include <string.h> // memset

void print_test_struct() {
   printf("sTestStruct == {\n");
   printf("   .color == { %u, %u, %u },\n", sTestStruct.color.r, sTestStruct.color.g, sTestStruct.color.b);
   printf("   .three_bit == %u\n", sTestStruct.three_bit);
   printf("   .five_bit == %u\n", sTestStruct.five_bit);
   printf("   .seven_bit == %u\n", sTestStruct.seven_bit);
   printf("   .three_to_seven == %u\n", sTestStruct.three_to_seven);
   printf("   .names == {\n");
   printf("      \"%.7s\",\n", sTestStruct.names[0]);
   printf("      \"%.7s\",\n", sTestStruct.names[1]);
   printf("   },\n");
   printf("   .foe_name = \"%.7s\",\n", sTestStruct.foe_name);
   printf("   .single_element_array_1D == { %u },\n", sTestStruct.single_element_array_1D[0]);
   printf("   .single_element_array_2D == { { %u } },\n", sTestStruct.single_element_array_2D[0][0]);
   printf("   .single_element_array_3D == { { { %u } } },\n", sTestStruct.single_element_array_3D[0][0][0]);
   printf("   .array == {\n");
   for(int i = 0; i < 3; ++i)
      printf("      %u,\n", sTestStruct.array[i]);
   printf("   },\n");
   printf("   .boolean == %u,\n", sTestStruct.boolean);
   printf("   .decimals == {\n");
   for(int i = 0; i < 3; ++i)
      printf("      %f,\n", sTestStruct.decimals[i]);
   printf("   },\n");
   printf("   .default_names == {\n");
   printf("      \"%.7s\",\n", sTestStruct.default_names.a);
   printf("      \"%.7s\",\n", sTestStruct.default_names.b);
   printf("      \"%.7s\",\n", sTestStruct.default_names.c);
   printf("      \"%.7s\",\n", sTestStruct.default_names.d);
   printf("      \"%.7s\",\n", sTestStruct.default_names.e);
   printf("      \"%.7s\",\n", sTestStruct.default_names.f);
   printf("      \"%.7s\",\n", sTestStruct.default_names.g);
   printf("   },\n");
   printf("   .xml_tricky_test = \"%.12s\",\n", sTestStruct.xml_tricky_test);
   printf("   .xml_tricky_test_2 = \"%.12s\",\n", sTestStruct.xml_tricky_test_2);
   printf("}\n");
   
   printf("*sTestStructAltPtr == {\n");
   printf("   array_a == {\n");
   printf("      ");
   for(int i = 0; i < 14; ++i) {
      printf("%u, ", sTestStructAltPtr->array_a[i]);
   }
   printf("\n");
   printf("   },\n");
   printf("   array_b == {\n");
   printf("      ");
   for(int i = 0; i < 18; ++i) {
      printf("%u, ", sTestStructAltPtr->array_b[i]);
   }
   printf("\n");
   printf("   },\n");
   printf("}\n");
   
   printf("*sTestStructWholePtr == {\n");
   printf("   array_a == {\n");
   printf("      ");
   for(int i = 0; i < 14; ++i) {
      printf("%u, ", sTestStructWholePtr->array_a[i]);
   }
   printf("\n");
   printf("   },\n");
   printf("}\n");
}

#include <stdlib.h> // malloc

const unsigned int offset_of_boolean;
const unsigned int sector_of_boolean;
#pragma lu_bitpack serialized_offset_to_constant    offset_of_boolean sTestStruct.boolean
#pragma lu_bitpack serialized_sector_id_to_constant sector_of_boolean sTestStruct.boolean

int main() {
   u8 sector_buffers[SECTOR_COUNT][SECTOR_SIZE] = { 0 };
   
   //
   // Set up initial test data.
   //
   sTestStruct.color.r = 0xFF;
   sTestStruct.color.g = 0x74;
   sTestStruct.color.b = 0x00;
   sTestStruct.three_bit = 3;
   sTestStruct.five_bit = 1 << 4;
   sTestStruct.seven_bit = 127;
   sTestStruct.three_to_seven = 5;
   memcpy(&sTestStruct.names[0], "ABCDEFG", 7);
   memcpy(&sTestStruct.names[1], "HIJKLMN", 7);
   memcpy(&sTestStruct.foe_name, "TUVWXYZ", 7);
   sTestStruct.single_element_array_1D[0] = 1;
   sTestStruct.single_element_array_2D[0][0] = 2;
   sTestStruct.single_element_array_3D[0][0][0] = 3;
   sTestStruct.array[0] = 16;
   sTestStruct.array[1] = 17;
   sTestStruct.array[2] = 18;
   sTestStruct.boolean = 1;
   sTestStruct.decimals[0] = 1.0;
   sTestStruct.decimals[1] = 2.5;
   sTestStruct.decimals[2] = 100.0;
   memset(&sTestStruct.default_names, 0, sizeof(sTestStruct.default_names));
   memset(sTestStruct.xml_tricky_test, 0, sizeof(sTestStruct.xml_tricky_test));
   memset(sTestStruct.xml_tricky_test_2, 0, sizeof(sTestStruct.xml_tricky_test_2));
   
   sTestStructAltPtr = (struct TestStructAlt*) malloc(sizeof(struct TestStructAlt));
   memset(sTestStructAltPtr, 0, sizeof(struct TestStructAlt));
   sTestStructAltPtr->array_a[5] = 55;
   sTestStructAltPtr->array_b[5] = 55;
   
   sTestStructWholePtr = (struct TestStructWhole*) malloc(sizeof(struct TestStructWhole));
   memset(sTestStructWholePtr, 0, sizeof(struct TestStructWhole));
   sTestStructWholePtr->array_a[5] = 33;
   
   const char* divider = "====================================================\n";
   
   printf(divider);
   printf("Variables:\n");
   printf("sector_of_boolean  == %u\n", sector_of_boolean);
   printf("offset_of_boolean  == %u\n", offset_of_boolean);
   {  // Test locals.
      #pragma lu_bitpack serialized_sector_id_to_constant sector_of_foe_name sTestStruct.foe_name
      #pragma lu_bitpack serialized_offset_to_constant    offset_of_foe_name sTestStruct.foe_name
      printf("sector_of_foe_name == %u\n", sector_of_foe_name);
      printf("offset_of_foe_name == %u\n", offset_of_foe_name);
      //
      // Test array slicing.
      //
      #pragma lu_bitpack serialized_sector_id_to_constant array_a_5_sector sTestStructAltPtr->array_a[5]
      #pragma lu_bitpack serialized_offset_to_constant    array_a_5_offset sTestStructAltPtr->array_a[5]
      printf("array_a_5_sector == %u\n", array_a_5_sector);
      printf("array_a_5_offset == %u\n", array_a_5_offset);
      #pragma lu_bitpack serialized_sector_id_to_constant array_b_5_sector sTestStructAltPtr->array_b[5]
      #pragma lu_bitpack serialized_offset_to_constant    array_b_5_offset sTestStructAltPtr->array_b[5]
      printf("array_b_5_sector == %u\n", array_b_5_sector);
      printf("array_b_5_offset == %u\n", array_b_5_offset);
      //
      // Test array slicing in a pointed-to struct
      //
      #pragma lu_bitpack serialized_sector_id_to_constant array_w_5_sector sTestStructWholePtr->array_a[5]
      #pragma lu_bitpack serialized_offset_to_constant    array_w_5_offset sTestStructWholePtr->array_a[5]
      printf("array_w_5_sector == %u\n", array_w_5_sector);
      printf("array_w_5_offset == %u\n", array_w_5_offset);
   }
   
   printf(divider);
   printf("Test data:\n");
   printf(divider);
   print_test_struct();
   printf("\n");
   
   //
   // Perform save.
   //
   memset(&sector_buffers, '~', sizeof(sector_buffers));
   for(int i = 0; i < __lu_bitpack_sector_count; ++i) {
      printf("Saving sector %u...\n", i);
      generated_save(sector_buffers[i], i);
   }
   printf("All sectors saved. Wiping sTestStruct...\n");
   memset(&sTestStruct, 0xCC, sizeof(sTestStruct));
   printf("Wiped. Reviewing data...\n\n");
   
   for(int i = 0; i < __lu_bitpack_sector_count; ++i) {
      printf("Sector %u saved:\n", i);
      print_buffer(sector_buffers[i], sizeof(sector_buffers[i]));
      printf("Sector %u read:\n", i);
      generated_read(sector_buffers[i], i);
      print_test_struct();
   }
   
   printf(divider);
   printf("Values extracted based on variables:\n");
   {
      const unsigned int array_a_5_sector;
      const unsigned int array_a_5_offset;
      const unsigned int array_b_5_sector;
      const unsigned int array_b_5_offset;
      const unsigned int array_w_5_sector;
      const unsigned int array_w_5_offset;
      #pragma lu_bitpack serialized_sector_id_to_constant array_a_5_sector sTestStructAltPtr->array_a[5]
      #pragma lu_bitpack serialized_offset_to_constant    array_a_5_offset sTestStructAltPtr->array_a[5]
      #pragma lu_bitpack serialized_sector_id_to_constant array_b_5_sector sTestStructAltPtr->array_b[5]
      #pragma lu_bitpack serialized_offset_to_constant    array_b_5_offset sTestStructAltPtr->array_b[5]
      #pragma lu_bitpack serialized_sector_id_to_constant array_w_5_sector sTestStructWholePtr->array_a[5]
      #pragma lu_bitpack serialized_offset_to_constant    array_w_5_offset sTestStructWholePtr->array_a[5]
      {
         struct lu_BitstreamState state;
         lu_BitstreamInitialize(&state, sector_buffers[sector_of_boolean]);
         state.target += offset_of_boolean / 8;
         state.shift   = offset_of_boolean % 8;
         bool8 v = lu_BitstreamRead_bool(&state);
         printf("sTestStruct.boolean == %u\n", v);
      }
      {
         struct lu_BitstreamState state;
         lu_BitstreamInitialize(&state, sector_buffers[array_a_5_sector]);
         state.target += array_a_5_offset / 8;
         state.shift   = array_a_5_offset % 8;
         u8 v = lu_BitstreamRead_u8(&state, 6);
         printf("sTestStructAltPtr->array_a[5] == %u\n", v);
      }
      {
         struct lu_BitstreamState state;
         lu_BitstreamInitialize(&state, sector_buffers[array_b_5_sector]);
         state.target += array_b_5_offset / 8;
         state.shift   = array_b_5_offset % 8;
         u8 v = lu_BitstreamRead_u8(&state, 6);
         printf("sTestStructAltPtr->array_b[5] == %u\n", v);
      }
      {
         struct lu_BitstreamState state;
         lu_BitstreamInitialize(&state, sector_buffers[array_w_5_sector]);
         state.target += array_w_5_offset / 8;
         state.shift   = array_w_5_offset % 8;
         u8 v = lu_BitstreamRead_u8(&state, 6);
         printf("sTestStructWholePtr->array_a[5] == %u\n", v);
      }
   }
   
   return 0;
}