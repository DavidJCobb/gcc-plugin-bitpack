#pragma once
#include <gcc-plugin.h>
#include "gcc_headers/plugin-version.h"

// IS_BITINT_TYPE_DEFINED
#if GCCPLUGIN_VERSION_MAJOR >= 14 && GCCPLUGIN_VERSION_MINOR >= 1
   // GCC PR102989
   // https://github.com/gcc-mirror/gcc/commit/4f4fa2501186e43d115238ae938b3df322c9e02a
   #define IS_BITINT_TYPE_DEFINED 1
#else
   #define IS_BITINT_TYPE_DEFINED 0
#endif
