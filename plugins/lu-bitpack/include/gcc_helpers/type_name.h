#pragma once
#include <string>
#include <string_view>
// GCC:
#include <gcc-plugin.h> 
#include <tree.h>

namespace gcc_helpers {
   extern std::string type_name(tree);
}