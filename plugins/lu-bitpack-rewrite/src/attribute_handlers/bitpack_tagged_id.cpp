#include "attribute_handlers/bitpack_tagged_id.h"
#include <cinttypes> // PRIdMAX and friends
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include <diagnostic.h>
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_tagged_id(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      helpers::bp_attr_context context(node_ptr, name, flags);
      if (TREE_CODE(node_ptr[0]) != FIELD_DECL) {
         context.report_error("can only be applied to data members in a union type");
      } else {
         //
         // DECL_CONTEXT isn't set until after attributes are applied, so we can't 
         // validate that we're being applied to a union member right now.
         //
      }
      
      std::optional<intmax_t> v;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && TREE_CODE(next) == INTEGER_CST) {
            v = gw::expr::integer_constant::from_untyped(next).value<intmax_t>();
         }
      }
      if (!v.has_value()) {
         context.report_error("argument must be an integer constant");
      }
      
      if (context.has_any_errors()) {
         *no_add_attrs = true;
      }
      return NULL_TREE;
   }
}