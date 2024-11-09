
/*

   TESTCASE: Bad attributes.

*/

#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool
#include <stdio.h>
#include <string.h> // memset

#pragma lu_bitpack enable

//
// START OF BITPACKING OPTIONS
//

#define LU_BP_AS_OPAQUE_BUFFER __attribute__((lu_bitpack_as_opaque_buffer))
#define LU_BP_BITCOUNT(n)      __attribute__((lu_bitpack_bitcount(n)))
#define LU_BP_INHERIT(name)    __attribute__((lu_bitpack_inherit(name)))
#define LU_BP_MINMAX(x,y)      __attribute__((lu_bitpack_range(x, y)))
#define LU_BP_MIN_0_MAX(n)     __attribute__((lu_bitpack_range(0, n)))
#define LU_BP_OMIT             __attribute__((lu_bitpack_omit))
#define LU_BP_STRING_UT        __attribute__((lu_bitpack_string))
#define LU_BP_STRING_WT        __attribute__((lu_bitpack_string("with-terminator")))
#define LU_BP_TRANSFORM(pre_pack, post_unpack) \
   __attribute__((lu_bitpack_transform("pre_pack="#pre_pack",post_unpack="#post_unpack)))

struct NestedStruct;
struct PackedNestedStruct;
void MapNestedStructForSave(const struct NestedStruct* src, struct PackedNestedStruct* dst);
void MapNestedStructForLoad(struct NestedStruct* dst, const struct PackedNestedStruct* src);

#pragma lu_bitpack set_options ( \
   sector_count=9, \
   sector_size=8,  \
   bool_typename            = bool8, \
   buffer_byte_typename     = void, \
   bitstream_state_typename = lu_BitstreamState, \
   string_char_typename     = u8, \
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

#pragma lu_bitpack heritable integer "$23bit" ( bitcount = 23 )
#pragma lu_bitpack heritable integer "$24bit" ( bitcount = 24 )

//
// END OF BITPACKING OPTIONS
//

LU_BP_INHERIT("$23bit") typedef u32 u32_packed_23;
typedef u32_packed_23 transitive_u23;
//#pragma lu_bitpack debug_dump_identifier u32_packed_23

enum ItemType {
   kItem_None = 0,
   
   kItem_WoodenSword,
   kItem_StoneSword,
   kItem_IronSword,
   kItem_GoldSword,
   kItem_DiamondSword,
   MAX_SWORD = kItem_DiamondSword,
   
   kItem_IronArmor,
   kItem_GoldArmor,
   kItem_DiamondArmor,
   MAX_ARMOR = kItem_DiamondArmor,
   
   ITEMS_COUNT,
};
const char* all_item_names[] = {
   "<none>",
   
   "Wooden Sword",
   "Stone Sword",
   "Iron Sword",
   "Gold Sword",
   "Diamond Sword",
   
   "Iron Armor",
   "Gold Armor",
   "Diamond Armor",
};

LU_BP_TRANSFORM(MapNestedStructForSave,MapNestedStructForLoad)
struct NestedStruct {
   enum ItemType weapons[5];
   enum ItemType armor[5];
};
struct PackedNestedStruct {
   LU_BP_MIN_0_MAX(MAX_SWORD) u8 weapons[5];
   LU_BP_MIN_0_MAX(MAX_ARMOR) u8 armor[5];
};

void MapNestedStructForSave(const struct NestedStruct* src, struct PackedNestedStruct* dst) {
   for(int i = 0; i < 5; ++i) {
      dst->weapons[i] = src->weapons[i];
   }
   for(int i = 0; i < 5; ++i) {
      if (src->armor[i])
         dst->armor[i] = src->armor[i] - MAX_SWORD;
      else
         dst->armor[i] = 0;
   }
}
void MapNestedStructForLoad(struct NestedStruct* dst, const struct PackedNestedStruct* src) {
   for(int i = 0; i < 5; ++i) {
      dst->weapons[i] = src->weapons[i];
   }
   for(int i = 0; i < 5; ++i) {
      if (src->armor[i])
         dst->armor[i] = src->armor[i] + MAX_SWORD;
      else
         dst->armor[i] = 0;
   }
}

static struct TestStruct {
   LU_BP_BITCOUNT(-1)  u32 a;
   LU_BP_BITCOUNT( 0)  u32 b;
   LU_BP_BITCOUNT( 1)  u32 c;
   
   LU_BP_MINMAX( 5, 0) u32 d;
   LU_BP_MINMAX( 0, 5) u32 e;
   LU_BP_MINMAX(-3, 0) u32 f;
   LU_BP_MINMAX( 0, 0) void* g;
   LU_BP_MINMAX( 0, 0) struct NestedStruct h;
   LU_BP_OMIT struct NestedStruct h_omit; // test: this one should actually work
   
   LU_BP_INHERIT("nonexistent!") u32 heritable_missing;
   LU_BP_INHERIT("")             u32 heritable_blank;
   LU_BP_INHERIT("$23bit") LU_BP_INHERIT("$24bit") u32 heritable_multiple;
   LU_BP_INHERIT("nonexistent!") LU_BP_INHERIT("$24bit") u32 heritable_multiple_one_missing;
} sTestStruct;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)

int main() {
   return 0;
}