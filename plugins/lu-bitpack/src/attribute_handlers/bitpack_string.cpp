#include "attribute_handlers/bitpack_string.h"
#include <optional>
#include <stdexcept>
#include "lu/strings/handle_kv_string.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/builtin_types.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_string(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      helpers::bp_attr_context context(node_ptr, name, flags);
      
      struct {
         std::optional<int>  length;
         std::optional<bool> with_terminator;
      } kv;
      
      {
         gw::expr::string_constant wrap;
         {
            auto next = TREE_VALUE(args);
            if (next != NULL_TREE && TREE_CODE(next) == STRING_CST) {
               wrap = gw::expr::string_constant::from_untyped(next);
            } else {
               context.report_error("argument, if specified, must be a string constant");
            }
         }
         if (!wrap.empty()) {
            try {
               lu::strings::handle_kv_string(wrap.value_view(), std::array{
                  lu::strings::kv_string_param{
                     .name      = "length",
                     .has_param = true,
                     .int_param = true,
                     .handler   = [&kv](std::string_view v, int vi) {
                        kv.length = vi;
                     },
                  },
                  lu::strings::kv_string_param{
                     .name      = "with-terminator",
                     .has_param = false,
                     .handler   = [&kv](std::string_view v, int vi) {
                        kv.with_terminator = true;
                     },
                  },
               });
            } catch (std::runtime_error& ex) {
               kv = {};
               
               std::string message = "argument string failed to parse: ";
               message += ex.what();
               context.report_error(message);
            }
         }
      }
      
      if (kv.length.has_value()) {
         auto value = *kv.length;
         if (value < 0) {
            context.report_error(
               "the string length cannot be zero or negative (seen: %r%d%R)",
               "quote",
               value
            );
         } else {
            auto args = tree_cons(
               NULL_TREE,
               gw::expr::integer_constant(
                  gw::builtin_types::get().size,
                  *kv.length
               ).as_untyped(),
               NULL_TREE
            );
            context.add_internal_attribute("lu bitpack computed string length", args);
         }
      }
      if (kv.with_terminator.has_value()) {
         auto args = tree_cons(
            NULL_TREE,
            *kv.with_terminator ? boolean_true_node : boolean_false_node,
            NULL_TREE
         );
         context.add_internal_attribute("lu bitpack computed string with-terminator", args);
      }
      
      if (context.has_any_errors()) {
         *no_add_attrs = true;
      }
      return NULL_TREE;
   }
}