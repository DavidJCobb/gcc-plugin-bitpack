#include <string>
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_helpers {
   // Pretty-print a typename in as close to C syntax as I care to get.
   extern std::string type_to_string(tree type);
}