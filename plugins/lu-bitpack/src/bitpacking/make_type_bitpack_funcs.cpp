#include "bitpacking/make_type_bitpack_funcs.h"

namespace codegen {
   extern std::pair<
      gcc_wrappers::decl::function, // read
      gcc_wrappers::decl::function  // write
   > make_type_bitpack_funcs(gcc_wrappers::type structure) {
      auto item = serialization_item::for_top_level_struct(structure);
      
      serialization_function_body body;
      body.item_list = item.expand();
      
      body.generate();
      
      for (auto* item : body.item_list)
         delete item;
   }
}