#pragma once
#include "lu/strings/zview.h"
#include "gcc_wrappers/attribute.h"

namespace bitpacking {
   extern bool attribute_attempted_on(gcc_wrappers::attribute_list, lu::strings::zview attr_name);
}