#include "codegen/whole_struct_function_dictionary.h"
#include <cassert>
namespace gw {
   using namespace gcc_wrappers;
}

namespace codegen {
   func_pair whole_struct_function_dictionary::get_functions_for(gw::type::base type) const{
      auto it = this->_functions.find(type);
      if (it == this->_functions.end())
         return {};
      return it->second;
   }
   
   void whole_struct_function_dictionary::add_functions_for(gw::type::base type, func_pair pair) {
      auto& slot = this->_functions[type];
      assert(slot.read.empty() && slot.save.empty());
      slot = pair;
   }
}