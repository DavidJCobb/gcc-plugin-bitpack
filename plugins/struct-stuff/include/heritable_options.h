#include <string_view>
#include "bitpack_options.h"

struct IntegralTypeinfo;

struct HeritableOptions {
   std::string_view name;
   struct {
      struct {
         bool is_untyped = false;
         IntegralTypeinfo* specific = nullptr;
      } integral;
      bool is_boolean = false;
      bool is_pointer = false;
      bool is_string  = false;
   } type;
   BitpackOptions options;
};