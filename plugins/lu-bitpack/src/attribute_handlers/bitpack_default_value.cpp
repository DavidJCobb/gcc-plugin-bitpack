#include "attribute_handlers/bitpack_default_value.h"
#include <cinttypes> // PRIdMAX and friends
#include <cstdint>
#include "lu/strings/printf_string.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/type/base.h"
#include "debugprint.h"
#include <print-tree.h>
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_default_value(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      auto data_node = TREE_VALUE(args);
      if (data_node == NULL_TREE) {
         return NULL_TREE;
      }
      
      gw::type::base type;
      {
         auto node = node_ptr[0];
         if (TYPE_P(node)) {
            type.set_from_untyped(node);
         } else {
            switch (TREE_CODE(node)) {
               case FIELD_DECL:
               case TYPE_DECL:
                  type.set_from_untyped(TREE_TYPE(node));
                  break;
               default:
                  error("%qE can only be applied to types and fields", name);
                  return NULL_TREE;
            }
         }
      }
      
      static_assert(false, "TODO: ints for integral types; strings for string types");
      static_assert(false, "TODO: validating strings requires knowing the string type in advance (i.e. we need to require that global options come before all attrs and can't be redefined)");
      
      return NULL_TREE;
   }
}