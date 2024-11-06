#pragma once
#include <string>
#include "gcc_wrappers/decl/function.h"

namespace bitpacking {
   extern bool can_be_transform_function(gcc_wrappers::decl::function);
   extern bool can_be_pre_pack_function(gcc_wrappers::decl::function);
   extern bool can_be_post_unpack_function(gcc_wrappers::decl::function);
   
   // Returns an error string in the event of a mismatch, or empty otherwise.
   extern std::string check_transform_functions_match_types(
      gcc_wrappers::decl::function pre_pack,
      gcc_wrappers::decl::function post_unpack
   );
}