#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include "bitpack_options.h"

struct IntegralTypeinfo {
   int64_t  min = 0;
   uint64_t max = 0;
   size_t   bytecount = 0;
   
   template<typename T>
   static constexpr IntegralTypeinfo from() {
      return IntegralTypeinfo{
         .min = (int64_t)  std::numeric_limits<T>::lowest(),
         .max = (uint64_t) std::numeric_limits<T>::max(),
         .bytecount = sizeof(T)
      };
   }
};

namespace known_integrals {
   inline constexpr const auto u8  = IntegralTypeinfo::from<uint8_t>();
   inline constexpr const auto u16 = IntegralTypeinfo::from<uint16_t>();
   inline constexpr const auto u32 = IntegralTypeinfo::from<uint32_t>();
   inline constexpr const auto u64 = IntegralTypeinfo::from<uint64_t>();
   inline constexpr const auto s8  = IntegralTypeinfo::from<int8_t>();
   inline constexpr const auto s16 = IntegralTypeinfo::from<int16_t>();
   inline constexpr const auto s32 = IntegralTypeinfo::from<int32_t>();
   inline constexpr const auto s64 = IntegralTypeinfo::from<int64_t>();
   
   inline constexpr const auto all = std::array{
      &u8, &u16, &u32, &u64,
      &s8, &s16, &s32, &s64,
   };
};

enum class fundamental_type {
   unknown,
   
   boolean,
   enumeration,
   integer,
   object, // struct or union
   pointer,
   reference,
   string,
};

struct DataMember {
   std::string name;
   struct {
      std::string name; // for struct, union, or enum member (see `is_enum`)
      const IntegralTypeinfo* integral = nullptr;
      fundamental_type fundamental = fundamental_type::unknown;
      
      bool is_const = false;
      
      std::optional<size_t> bitfield_bitcount;
      
      bool is_flexible_array = false;
      std::vector<size_t> array_indices; // e.g. 3,4,5 if this is a call to serialize p_Struct.foo[3][4][5]
      
      struct {
         int64_t  min = 0;
         uint32_t max = 0;
      } enum_bounds;
   } type;
   BitpackOptions options;
};