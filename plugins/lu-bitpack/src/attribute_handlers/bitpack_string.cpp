#include "attribute_handlers/bitpack_string.h"
#include <optional>
#include <stdexcept>
#include "lu/strings/handle_kv_string.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/builtin_types.h"
#include <stringpool.h> // dependency for <attribs.h>
#include <attribs.h> // decl_attributes
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   static void _reassemble_parameters(
      tree* node_ptr,
      tree  name,
      gw::expr::string_constant data,
      int   flags,
      //
      helpers::bp_attr_context& context
   ) {
      struct {
         std::optional<int>  length;
         std::optional<bool> with_terminator;
      } kv;
      try {
         lu::strings::handle_kv_string(data.value_view(), std::array{
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
         std::string message = "argument failed to parse: ";
         message += ex.what();
         context.report_error(message);
         return;
      }
      if (kv.length.has_value()) {
         auto v = *kv.length;
         if (v < 0) {
            context.report_error(
               "the string length cannot be zero or negative (seen: %r%d%R)",
               "quote",
               v
            );
            return;
         }
      }
      
      if (context.has_any_errors()) {
         return;
      }
      
      tree node_len = NULL_TREE;
      tree node_wt  = NULL_TREE;
      if (kv.length.has_value()) {
         node_len = gw::expr::integer_constant(
            gw::builtin_types::get().size,
            *kv.length
         ).as_untyped();
      }
      if (kv.with_terminator.has_value()) {
         node_wt = *kv.with_terminator ? boolean_true_node : boolean_false_node;
      }
      
      tree args = tree_cons(
         NULL_TREE,
         node_len,
         tree_cons(
            NULL_TREE,
            node_wt,
            NULL_TREE
         )
      );
      
      auto attr_node = tree_cons(name, args, NULL_TREE);
      decl_attributes(
         &node_ptr[0],
         attr_node,
         flags | ATTR_FLAG_INTERNAL,
         node_ptr[1]
      );
   }

   extern tree bitpack_string(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      if (flags & ATTR_FLAG_INTERNAL) {
         return NULL_TREE;
      }
      if (list_length(args) > 1) {
         error("wrong number of arguments specified for %qE attribute", name);
         *no_add_attrs = true;
         return NULL_TREE;
      }
      
      helpers::bp_attr_context context(node_ptr, name, flags);
      context.check_and_report_applied_to_string();
      
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
            //
            // Parse the arguments and reconstruct them.
            //
            *no_add_attrs = true;
            _reassemble_parameters(
               node_ptr,
               name,
               wrap,
               flags,
               context
            );
            return NULL_TREE;
         }
      }
      //
      // Case of the pragma specified with no args.
      //
      if (context.has_any_errors()) {
         *no_add_attrs = true;
      }
      return NULL_TREE;
   }
}