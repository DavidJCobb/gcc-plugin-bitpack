
#include <stdint.h>

constexpr int player_name_length = 7;
typedef char player_name_type [player_name_length];

#pragma lu_bitpack debug_dump_identifier player_name_length
#pragma lu_bitpack debug_dump_identifier player_name_type

struct TestStruct {
   char name_a[7];
   char name_b[player_name_length];
   player_name_type name_c;
};

#pragma lu_bitpack debug_dump_identifier TestStruct
#pragma lu_bitpack debug_dump_identifier TestStruct::name_a
#pragma lu_bitpack debug_dump_identifier TestStruct::name_b
#pragma lu_bitpack debug_dump_identifier TestStruct::name_c

static struct TestInstance {
   int32_t foo;
   int     bar;
   int     bitfield_1 : 5;
   int     bitfield_2 : player_name_length;
} sTestInstance;

#pragma lu_bitpack debug_dump_identifier sTestInstance::foo // should fail
#pragma lu_bitpack debug_dump_identifier sTestInstance.foo  // should work
#pragma lu_bitpack debug_dump_identifier sTestInstance.bitfield_1
#pragma lu_bitpack debug_dump_identifier sTestInstance.bitfield_2

// a handful of random other tests thrown in here for temporary convenience:

static struct TestInstance* sTestPointer = &sTestInstance;

#pragma lu_bitpack debug_dump_identifier sTestPointer.bar  // should fail
#pragma lu_bitpack debug_dump_identifier sTestPointer->bar // should work

enum {
   ENUM_CONSTANT_0 = 0,
   ENUM_CONSTANT_1 = 1,
};

#pragma lu_bitpack debug_dump_identifier ENUM_CONSTANT_0