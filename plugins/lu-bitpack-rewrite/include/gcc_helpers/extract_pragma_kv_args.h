#pragma once
#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
// GCC:
#include <gcc-plugin.h>
#include <c-family/c-pragma.h>

struct cppreader;

namespace gcc_helpers {
   using pragma_kv_value = std::variant<
      std::monostate,
      std::string, // identifier
      std::string, // string literal
      intmax_t
   >;
   using pragma_kv_set = std::unordered_map<
      std::string,
      pragma_kv_value
   >;
   
   extern pragma_kv_set extract_pragma_kv_args(
      const char* pragma_name, // for error reporting
      cpp_reader* reader
   );
}