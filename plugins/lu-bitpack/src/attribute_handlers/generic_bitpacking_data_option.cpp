#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "attribute_handlers/generic_type_or_decl.h"
#include "basic_global_state.h"
#include <diagnostic.h>

namespace attribute_handlers {
   extern tree generic_bitpacking_data_option(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_type_or_decl(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      auto& state = basic_global_state::get();
      state.any_attributes_seen = true;
      if (!state.enabled) {
         state.any_attributes_missed = true;
         *no_add_attrs = true;
      } else {
         if (!state.global_options_seen) {
            auto& flag = state.errors_reported.data_options_before_global_options;
            if (!flag) {
               flag = true;
               error("bitpacking: data options seen before global options; you need to use %<#pragma lu_bitpack set_options%> first so we can validate attributes against them");
            }
         }
      }
      
      return NULL_TREE;
   }
}