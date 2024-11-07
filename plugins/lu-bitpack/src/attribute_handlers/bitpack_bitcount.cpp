#include "attribute_handlers/bitpack_bitcount.h"
#include <cinttypes> // PRIdMAX and friends
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_type_or_decl.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include <diagnostic.h>
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_bitcount(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_type_or_decl(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      bool error = false;
      
      helpers::bp_attr_context context(name, *node_ptr);
      error = !context.check_and_report_applied_to_integral();
      
      std::optional<intmax_t> v;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && TREE_CODE(next) == INTEGER_CST) {
            v = gw::expr::integer_constant::from_untyped(next).value<intmax_t>();
         }
      }
      if (!v.has_value()) {
         context.report_error("argument must be an integer constant");
         error = true;
      } else {
         auto bc = *v;
         if (bc <= 0) {
            context.report_error("argument cannot be zero or negative (seen: %<%" PRIdMAX "%>)", bc);
            error = true;
         } else if (bc > (intmax_t)sizeof(size_t) * 8) {
            warning(OPT_Wattributes, "%qE attribute specifies an unusually large bitcount; is this intentional? (seen: %<%" PRIdMAX "%>)", name, bc);
         }
      }
      
      auto dst_opt = context.get_destination();
      if (dst_opt.has_value()) {
         auto dst = *dst_opt;
         if (error) {
            //*no_add_attrs = true; // actually, do add the argument, so codegen can fail later on
            dst.set_bitcount(bitpacking::data_options::processed_bitpack_attributes::invalid_tag{});
         } else {
            *no_add_attrs = false;
            dst.set_bitcount((size_t)*v);
         }
      }
      return NULL_TREE;
   }
}