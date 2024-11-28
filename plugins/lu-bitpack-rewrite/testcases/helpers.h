#ifndef GUARD_LU_TEST_HELPERS
#define GUARD_LU_TEST_HELPERS

#define LU_BP_AS_OPAQUE_BUFFER __attribute__((lu_bitpack_as_opaque_buffer))
#define LU_BP_BITCOUNT(n)      __attribute__((lu_bitpack_bitcount(n)))
#define LU_BP_DEFAULT(x)       __attribute__((lu_bitpack_default_value(x)))
#define LU_BP_OMIT             __attribute__((lu_bitpack_omit))
#define LU_BP_STRING           __attribute__((lu_bitpack_string))
#define LU_BP_STRING_NT        LU_BP_STRING
#define LU_BP_STRING_UT        __attribute__((lu_nonstring)) LU_BP_STRING
#define LU_BP_TRANSFORM(pre_pack, post_unpack) \
   __attribute__((lu_bitpack_transforms("pre_pack=" #pre_pack ",post_unpack=" #post_unpack)))

#define LU_BITPACK_CATEGORY(name) __attribute__((lu_bitpack_stat_category(name)))

// Not yet implemented.
// Indicate the value that acts as a union's tag, when that value is not 
// inside of [all of the members of] the union itself. The union must be 
// located somewhere inside of a containing struct (named tag or typedef) 
// and the attribute's argument must be a string denoting the name of a 
// member of said struct that appears before the union does.
#define LU_BP_UNION_TAG(n) __attribute__((lu_bitpack_union_external_tag( #n )))

// Not yet implemented.
// Indicate which member-of-member of this union acts as its tag.
// For example, `name` means that `__union.__any_member.name` is the tag.
#define LU_BP_UNION_INTERNAL_TAG(n) __attribute__((lu_bitpack_union_internal_tag( #n )))

// Not yet implemented.
// Use on a union member to specify the tag value that indicates that that 
// member is active. Must be an integral value.
#define LU_BP_TAGGED_ID(n) __attribute__((lu_bitpack_union_member_id(n)))

#include <ctype.h>
#include <stdio.h>

void print_buffer(const char* buffer, int size) {
   printf("Buffer:\n   ");
   for(int i = 0; i < size; ++i) {
      if (i && i % 8 == 0) {
         printf("\n   ");
      }
      printf("%02X ", (uint8_t) buffer[i]);
   }
   printf("\n");
}

void print_char(char c) {
   if (isprint(c)) {
      printf("'%c'", (uint8_t)(c & 0xFF));
   } else {
      printf("0x%02X", c & 0xFF);
   }
}

void print_top_level_string_member(const char* name, int size, const char* data) {
   printf("      .%s = {\n", name);
   for(int i = 0; i < size; ++i) {
      printf("         ");
      print_char(data[i]);
      printf(",\n");
   }
   printf("      },\n");
}

#endif