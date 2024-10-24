#include <optional>
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_helpers {
   extern std::optional<size_t> bitcount_of_type(tree type);
}