#include "codegen/instructions/single.h"
#include "gcc_wrappers/builtin_types.h"

namespace codegen::instructions {
   bool single::is_omitted_and_defaulted() const {
      bool omitted   = false;
      bool defaulted = false;
      for(auto& segm : this->value.segments) {
         auto  pair = segm.descriptor();
         auto* desc = pair.read;
         if (desc == nullptr)
            continue;
         if (desc->options.omit_from_bitpacking)
            omitted = true;
         if (desc->options.default_value_node != NULL_TREE)
            defaulted = true;
      }
      return omitted && defaulted;
   }
}