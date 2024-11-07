#include "attribute_handlers/bitpack_range.h"
#include <cinttypes> // PRIdMAX and friends
#include <cstdint>
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_type_or_decl.h"
#include "gcc_wrappers/expr/integer_constant.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_range(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_type_or_decl(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      bool error = false;
      
      helpers::bp_attr_context context(name, *node_ptr);
      error = !context.check_and_report_applied_to_integral();
      
      std::optional<intmax_t> min;
      std::optional<intmax_t> max;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && TREE_CODE(next) == INTEGER_CST) {
            min = gw::expr::integer_constant::from_untyped(next).value<intmax_t>();
         }
         
         args = TREE_CHAIN(args);
         if (args != NULL_TREE) {
            auto next = TREE_VALUE(args);
            if (next != NULL_TREE && TREE_CODE(next) == INTEGER_CST) {
               max = gw::expr::integer_constant::from_untyped(next).value<intmax_t>();
            }
         }
      }
      if (!min.has_value() || !max.has_value()) {
         constexpr const char* const message = "%s argument (%s value) must be an integer constant";
         if (!min.has_value()) {
            context.report_error(message, "first", "minimum");
            error = true;
         }
         if (!max.has_value()) {
            context.report_error(message, "second", "maximum");
            error = true;
         }
      } else {
         if (*min > *max) {
            context.report_error("minimum value (first argument) is greater than the maximum value (second argument) i.e. %" PRIdMAX " is greater than %" PRIdMAX "", *min, *max);
            error = true;
         }
      }
      
      auto dst_opt = context.get_destination();
      if (dst_opt.has_value()) {
         auto dst = *dst_opt;
         if (error) {
            //*no_add_attrs = true; // actually, do add the argument, so codegen can fail later on
            dst.set_min(bitpacking::data_options::processed_bitpack_attributes::invalid_tag{});
            dst.set_max(bitpacking::data_options::processed_bitpack_attributes::invalid_tag{});
         } else {
            *no_add_attrs = false;
            dst.set_min(*min);
            dst.set_max(*max);
         }
      }
      return NULL_TREE;
   }
}