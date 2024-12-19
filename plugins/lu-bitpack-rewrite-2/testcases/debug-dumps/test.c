
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
   LU_BP_DEFAULT("Silver") player_name enemy_name;
   
   u8 single_element_array_1D[1];
   u8 single_element_array_2D[1][1];
   u8 single_element_array_3D[1][1][1];
   
   u5 array[3];
   
   bool8 boolean;
   
   LU_BP_AS_OPAQUE_BUFFER float decimals[3];
   
   struct {
      LU_BP_OMIT LU_BP_DEFAULT("May")     player_name hoenn;
      LU_BP_OMIT LU_BP_DEFAULT("Dawn")    player_name sinnoh;
      LU_BP_OMIT LU_BP_DEFAULT("Lyra")    player_name johto_modern;
      LU_BP_OMIT LU_BP_DEFAULT("Hilda")   player_name unova_a;
      LU_BP_OMIT LU_BP_DEFAULT("Serena")  player_name kalos;
      LU_BP_OMIT LU_BP_DEFAULT("Juliana") player_name paldea;
      // Test to verify that the previous name doesn't overflow:
      LU_BP_OMIT LU_BP_DEFAULT("Rosa")    player_name unova_b;
   } default_names;
   
   LU_BP_OMIT LU_BP_DEFAULT("<\"xml\">\n<>") LU_BP_STRING char xml_tricky_test[12];
   
   // Testcase: default value is string; value is not marked as a string.
   LU_BP_OMIT LU_BP_DEFAULT("<\"xml\">\n<>") char xml_tricky_test_2[12];
   
   NamedByTypedefOnly named_by_typedef_only;
   struct NamedByTag  using_tag_name;
   NamedByTypedef     using_typedef_name;
} sTestStruct;

#pragma lu_bitpack debug_dump_identifier TestStruct
#pragma lu_bitpack debug_dump_identifier sTestStruct
#pragma lu_bitpack debug_dump_identifier sTestStruct::boolean
#pragma lu_bitpack debug_dump_identifier sTestStruct::names
#pragma lu_bitpack debug_dump_identifier sTestStruct::named_by_typedef_only
#pragma lu_bitpack debug_dump_identifier NamedByTypedefOnly
#pragma lu_bitpack debug_dump_identifier NamedByTag

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)
#pragma lu_bitpack debug_dump_function generated_read
#pragma lu_bitpack debug_dump_identifier __lu_bitpack_sector_count
#pragma lu_bitpack debug_dump_identifier __lu_bitpack_max_sector_size
