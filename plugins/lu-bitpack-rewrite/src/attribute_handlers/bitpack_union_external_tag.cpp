#include "attribute_handlers/bitpack_union_external_tag.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/untagged_union.h"
#include <stringpool.h> // get_identifier
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_union_external_tag(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      if (flags & ATTR_FLAG_INTERNAL) {
         return NULL_TREE;
      }
      
      helpers::bp_attr_context context(node_ptr, name, flags);
      
      if (context.target_field().empty()) {
         context.report_error("can only be applied to data members whose types are unions");
      } else {
         auto type = context.type_of_target();
         while (type.is_array())
            type = type.as_array().value_type();
         if (!type.is_union()) {
            context.report_error("can only be applied to data members whose types are unions");
         }
      }
      
      gw::expr::string_constant data;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && TREE_CODE(next) == STRING_CST) {
            data = gw::expr::string_constant::from_untyped(next);
         } else {
            context.report_error("argument must be a string constant");
         }
      }
      
      *no_add_attrs = true;
      if (!data.empty()) {
         auto view = data.value_view();
         if (view.empty()) {
            context.report_error("tag identifier cannot be blank");
         }
      }
      
      *no_add_attrs = true;
      if (!context.has_any_errors()) {
         auto view = data.value_view();
         
         gw::list_node args;
         args.append(NULL_TREE, get_identifier(view.data()));
         context.reapply_with_new_args(args);
      }
      return NULL_TREE;
   }
}