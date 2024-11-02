#pragma once
#include <optional>
#include <utility>
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_helpers {
   extern std::pair<
      std::optional<intmax_t>,
      std::optional<uintmax_t>
   > type_min_max(tree type);
}