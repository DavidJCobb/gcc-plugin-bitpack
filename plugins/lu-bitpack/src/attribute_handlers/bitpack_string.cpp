#include "attribute_handlers/bitpack_string.h"
#include <optional>
#include <stdexcept>
#include "lu/strings/handle_kv_string.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/builtin_types.h"
#include "gcc_wrappers/list_node.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_string(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      helpers::bp_attr_context context(node_ptr, name, flags);
      context.check_and_report_applied_to_string();
      
      auto type = context.bitpacking_value_type();
      if (type.is_array()) {
         auto extent_opt = type.as_array().extent();
         if (!extent_opt.has_value()) {
            context.report_error("applied to a variable-length array; VLAs are not supported");
         } else {
            size_t extent = *extent_opt;
            if (extent == 0) {
               context.report_error("applied to a zero-length array");
            } else {
               bool optional_terminator = context.get_existing_attributes().has_attribute("nonstring");
               
               if (!optional_terminator && extent == 1) {
                  context.report_error("applied to a zero-length string (array length is 1; terminator byte is required; ergo no room for any actual string content)");
               }
            }
         }
      }
      
      if (context.has_any_errors()) {
         *no_add_attrs = true;
      }
      return NULL_TREE;
   }
}