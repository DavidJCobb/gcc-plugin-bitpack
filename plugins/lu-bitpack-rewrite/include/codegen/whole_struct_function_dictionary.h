#pragma once
#include <unordered_map>
#include "codegen/func_pair.h"
#include "gcc_wrappers/type/base.h"

namespace codegen {
   class whole_struct_function_dictionary {
      protected:
         std::unordered_map<gcc_wrappers::type::base, func_pair> _functions;
         
      public:
         func_pair get_functions_for(gcc_wrappers::type::base) const;
         
         // Asserts that functions don't already exist for the given struct type.
         void add_functions_for(gcc_wrappers::type::base, func_pair);
   };
}