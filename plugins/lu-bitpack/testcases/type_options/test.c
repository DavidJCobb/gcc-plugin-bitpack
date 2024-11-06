
/*

   TESTCASE: Generating an XML report of the bitpack format.

*/

#include "types.h"
#include "bitstreams.h"

#include <stdbool.h> // bool
#include <stdio.h>
#include <string.h> // memset

#define LU_BP_BITCOUNT(n)   __attribute__((lu_bitpack_bitcount(n)))
#define LU_BP_INHERIT(name) __attribute__((lu_bitpack_inherit(name)))
#define LU_BP_MINMAX(x,y)   __attribute__((lu_bitpack_range(x, y)))
#define LU_BP_MAX(n)        __attribute__((lu_bitpack_range(0, n)))

#define LU_BP_TRANSFORM(pre_pack, post_unpack) \
   __attribute__((lu_bitpack_transform("pre_pack="#pre_pack",post_unpack="#post_unpack)))

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

#pragma lu_bitpack heritable integer "$24bit" ( bitcount = 24 )

LU_BP_INHERIT("$24bit") typedef u32 u32_packed_24;
#pragma lu_bitpack debug_dump_identifier u32_packed_24

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

LU_BP_TRANSFORM(MapNestedStructForSave,MapNestedStructForLoad) struct NestedStruct {
   enum ItemType weapons[5];
   enum ItemType armor[5];
};
struct PackedNestedStruct {
   LU_BP_MAX(MAX_SWORD) u8 weapons[5];
   LU_BP_MAX(MAX_ARMOR) u8 armor[5];
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
   bool8 a;
   bool8 b;
   bool8 c;
   bool8 d;
   bool8 e;
   LU_BP_BITCOUNT(5)  u8  f;
   LU_BP_BITCOUNT(3)  u8  g;
   LU_BP_BITCOUNT(12) u16 h;
   LU_BP_INHERIT("$24bit") u32 i;
   u32_packed_24 j;
   LU_BP_BITCOUNT(24) u32 k;
   LU_BP_BITCOUNT(7)  u8  l[7];
   struct NestedStruct inventory;
} sTestStruct;

extern void generated_read(const u8* src, int sector_id);
extern void generated_save(u8* dst, int sector_id);

#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct             \
)

void print_buffer(const u8* buffer, int size) {
   printf("Buffer:\n   ");
   for(int i = 0; i < size; ++i) {
      if (i && i % 8 == 0) {
         printf("\n   ");
      }
      printf("%02X ", buffer[i]);
   }
   printf("\n");
}
void print_data() {
   printf("   sTestStruct == {\n");
   printf("      .a = %u,\n", sTestStruct.a);
   printf("      .b = %u,\n", sTestStruct.b);
   printf("      .c = %u,\n", sTestStruct.c);
   printf("      .d = %u,\n", sTestStruct.d);
   printf("      .e = %u,\n", sTestStruct.e);
   printf("      .f = %u,\n", sTestStruct.f);
   printf("      .g = %u,\n", sTestStruct.g);
   printf("      .h = %u,\n", sTestStruct.h);
   printf("      .i = %u,\n", sTestStruct.i);
   printf("      .j = %u,\n", sTestStruct.j);
   printf("      .k = %u,\n", sTestStruct.k);
   printf("      .l = {\n");
   for(int i = 0; i < 7; ++i) {
      printf("         %u,\n", sTestStruct.l[i]);
   }
   printf("      },\n");
   printf("      .inventory = {\n");
   printf("         .weapons = {\n");
   for(int i = 0; i < 5; ++i) {
      printf("            ");
      int item = sTestStruct.inventory.weapons[i];
      if (item < ITEMS_COUNT) {
         printf(all_item_names[item]);
      } else {
         printf("%u", item);
      }
      printf("\n");
   }
   printf("         },\n");
   printf("         .armor = {\n");
   for(int i = 0; i < 5; ++i) {
      printf("            ");
      int item = sTestStruct.inventory.armor[i];
      if (item < ITEMS_COUNT) {
         printf(all_item_names[item]);
      } else {
         printf("%u", item);
      }
      printf("\n");
   }
   printf("         },\n");
   printf("      },\n");
   printf("   }\n");
}

#define TEST_SECTOR_COUNT 9
int main() {
   u8 sector_buffers[TEST_SECTOR_COUNT][64] = { 0 };
   
   const char* divider = "=========================================================\n";
   
   sTestStruct.a = true;
   sTestStruct.b = false;
   sTestStruct.c = true;
   sTestStruct.d = false;
   sTestStruct.e = true;
   sTestStruct.f = 0b11111;
   sTestStruct.g = 0b101;
   sTestStruct.h = 7;
   sTestStruct.i = 0b10101010101010;
   sTestStruct.j = 0b10101010101010;
   sTestStruct.k = 0b10101010101010;
   for(int i = 0; i < 7; ++i) {
      sTestStruct.l[i] = (i + 1) * 11;
   }
   memset(sTestStruct.inventory.weapons, 0, sizeof(sTestStruct.inventory.weapons));
   memset(sTestStruct.inventory.armor, 0, sizeof(sTestStruct.inventory.armor));
   sTestStruct.inventory.weapons[0] = kItem_IronSword;
   sTestStruct.inventory.weapons[1] = kItem_IronSword;
   sTestStruct.inventory.weapons[2] = kItem_DiamondSword;
   sTestStruct.inventory.weapons[3] = kItem_WoodenSword;
   sTestStruct.inventory.armor[0] = kItem_IronArmor;
   sTestStruct.inventory.armor[1] = kItem_GoldArmor;
   
   for(int i = 0; i < TEST_SECTOR_COUNT; ++i) {
      memset(sector_buffers[i], '~', sizeof(sector_buffers[i]));
   }
   
   printf(divider);
   printf("Test data to save:\n");
   printf(divider);
   print_data();
   printf("\n");
   printf(divider);
   printf("Testing generated functions...\n");
   printf(divider);
   
   for(int i = 0; i < TEST_SECTOR_COUNT; ++i) {
      generated_save(sector_buffers[i], i);
   }
   memset(&sTestStruct, 0, sizeof(sTestStruct));
   
   for(int i = 0; i < TEST_SECTOR_COUNT; ++i) {
      printf("Sector %u saved:\n", i);
      print_buffer(sector_buffers[i], sizeof(sector_buffers[i]));
      printf("Sector %u read:\n", i);
      generated_read(sector_buffers[i], i);
      print_data();
   }
   
   return 0;
}