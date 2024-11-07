#include "attribute_handlers/bitpack_inherit.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_type_or_decl.h"
#include "gcc_wrappers/expr/string_constant.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_inherit(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_type_or_decl(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      bool error = false;
      
      helpers::bp_attr_context context(name, *node_ptr);
      
      gw::expr::string_constant wrap;
      
      std::optional<intmax_t> v;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && TREE_CODE(next) == STRING_CST) {
            wrap = gw::expr::string_constant::from_untyped(next);
         } else {
            context.report_error("argument must be a non-empty string constant");
            error = true;
         }
      }
      if (!wrap.empty()) {
         auto name = wrap.value_view();
         if (name.empty()) {
            context.report_error("heritable options name cannot be blank");
            error = true;
         }
      }
      
      auto dst_opt = context.get_destination();
      if (dst_opt.has_value()) {
         auto dst = *dst_opt;
         if (error) {
            //*no_add_attrs = true; // actually, do add the argument, so codegen can fail later on
            dst.set_inherit(bitpacking::data_options::processed_bitpack_attributes::invalid_tag{});
         } else {
            *no_add_attrs = false;
            dst.set_inherit(wrap.as_untyped());
         }
      }
      return NULL_TREE;
   }
}