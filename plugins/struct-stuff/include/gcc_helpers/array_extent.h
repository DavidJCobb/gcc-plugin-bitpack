#include <optional>
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_helpers {
   extern std::optional<size_t> array_extent(tree type);
}