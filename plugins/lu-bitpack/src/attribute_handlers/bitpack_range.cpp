#include "attribute_handlers/bitpack_range.h"
#include <cinttypes> // PRIdMAX and friends
#include <cstdint>
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "gcc_wrappers/expr/integer_constant.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_range(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      helpers::bp_attr_context context(node_ptr, name, flags);
      context.check_and_report_applied_to_integral();
      
      gw::expr::integer_constant min_node;
      gw::expr::integer_constant max_node;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && TREE_CODE(next) == INTEGER_CST)
            min_node = gw::expr::integer_constant::from_untyped(next);
         
         args = TREE_CHAIN(args);
         if (args != NULL_TREE) {
            auto next = TREE_VALUE(args);
            if (next != NULL_TREE && TREE_CODE(next) == INTEGER_CST)
               max_node = gw::expr::integer_constant::from_untyped(next);
         }
      }
      std::optional<intmax_t> min = min_node.value<intmax_t>();
      std::optional<intmax_t> max = max_node.value<intmax_t>();
      
      constexpr const char* const absent_message = "%s argument (%s value) must be an integer constant";
      
      bool both = true;
      if (!min.has_value()) {
         both = false;
         context.report_error(absent_message, "first", "minimum");
      }
      if (!max.has_value()) {
         both = false;
         context.report_error(absent_message, "second", "maximum");
      }
      if (both) {
         if (*min > *max) {
            context.report_error(
               "minimum value (first argument) is greater than the maximum value (second argument) i.e. %r%" PRIdMAX "%R is greater than %r%" PRIdMAX "%R",
               "quote",
               *min,
               "quote",
               *max
            );
         }
      }
      
      if (context.has_any_errors()) {
         *no_add_attrs = true;
      }
      return NULL_TREE;
   }
}