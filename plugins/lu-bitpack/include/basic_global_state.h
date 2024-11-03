#pragma once
#include "lu/singleton.h"
#include "bitpacking/global_options.h"

class basic_global_state;
class basic_global_state : public lu::singleton<bitpacking_global_state> {
   public:
      struct {
         bitpacking::global_options::requested requested;
         bitpacking::global_options::computed  computed;
      } global_options;
};
