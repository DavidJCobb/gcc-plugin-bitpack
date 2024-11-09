#include "attribute_handlers/bitpack_inherit.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "bitpacking/heritable_options.h"
#include "gcc_wrappers/expr/string_constant.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_inherit(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      helpers::bp_attr_context context(node_ptr, name, flags);
      if (context.attribute_already_attempted()) {
         context.report_error("is present more than once; you can only set one heritable option set");
      }
      
      gw::expr::string_constant wrap;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && TREE_CODE(next) == STRING_CST) {
            wrap = gw::expr::string_constant::from_untyped(next);
         } else {
            context.report_error("argument must be a non-empty string constant");
         }
      }
      if (!wrap.empty()) {
         auto name = wrap.value_view();
         if (name.empty()) {
            context.report_error("heritable option set name cannot be blank");
         } else {
            auto* o = bitpacking::heritable_options_stockpile::get().options_by_name(name);
            if (!o)
               context.report_error("specifies a heritable option set %<%s%> that has not yet been defined", name.data());
         }
      }
      
      if (context.has_any_errors()) {
         *no_add_attrs = true;
      }
      return NULL_TREE;
   }
}