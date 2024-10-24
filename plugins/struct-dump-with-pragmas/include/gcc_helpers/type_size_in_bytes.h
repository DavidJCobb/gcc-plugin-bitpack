#include <optional>
// GCC:
#include <gcc-plugin.h> 
#include <tree.h>

namespace gcc_helpers {
   extern std::optional<size_t> type_size_in_bits(tree);
   extern std::optional<size_t> type_size_in_bytes(tree);
}