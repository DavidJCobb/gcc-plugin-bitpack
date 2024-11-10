#include "attribute_handlers/bitpack_default_value.h"
#include <cinttypes> // PRIdMAX and friends
#include <cstdint>
#include "lu/strings/printf_string.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/attribute.h"
#include "basic_global_state.h"
#include "debugprint.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   static void _check_and_report_real_value(helpers::bp_attr_context& context, tree node) {
      if (node == NULL_TREE)
         return;
      switch (TREE_CODE(node)) {
         case INTEGER_CST:
         case REAL_CST:
            return;
         default:
            break;
      }
      context.report_error("specifies a default value of an incorrect type");
   }
   static void _check_and_report_integral_value(helpers::bp_attr_context& context, tree node) {
      if (node == NULL_TREE)
         return;
      if (node == boolean_true_node || node == boolean_false_node)
         return;
      switch (TREE_CODE(node)) {
         case INTEGER_CST:
            return;
         default:
            break;
      }
      context.report_error("specifies a default value of an incorrect type");
   }
   static void _check_and_report_pointer_value(helpers::bp_attr_context& context, tree node) {
      if (node == NULL_TREE) {
         return;
      }
      {
         auto type = TREE_TYPE(node);
         if (type && TREE_CODE(type) == NULLPTR_TYPE)
            return;
      }
      if (!integer_zerop(node)) {
         context.report_error("uses a non-zero default value; the only permitted default value for a pointer is zero");
      }
   }
   static void _check_and_report_string_value(helpers::bp_attr_context& context, tree node) {
      if (node == NULL_TREE) {
         return;
      }
      if (TREE_CODE(node) != STRING_CST) {
         context.report_error("specifies a default value of an incorrect type");
         return;
      }
      
      auto type = context.bitpacking_value_type();
      if (type.is_array()) {
         auto data = gw::expr::string_constant::from_untyped(node);
         auto size = data.size_of();
         
         auto extent_opt = type.as_array().extent();
         if (extent_opt.has_value()) {
            auto extent    = *extent_opt;
            bool nonstring = context.get_existing_attributes().has_attribute("nonstring");
            if (nonstring)
               --size;
            
            if (size > extent) {
               context.report_error(
                  "specifies a default value that is too long (%r%u%R characters) to fit in the array (%r%u%R)",
                  "quote",
                  size,
                  "quote",
                  extent
               );
               if (!nonstring && size == extent + 1) {
                  context.report_note("if this array does not require a null terminator, then applying %<__attribute__((nonstring))%> before this attribute would allow this value to fit, while also allowing GCC to error-check certain string functions for you");
               }
            }
         }
      }
   }
   
   extern tree bitpack_default_value(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      *no_add_attrs = false;
      
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      helpers::bp_attr_context context(node_ptr, name, flags);
      
      auto value_node = TREE_VALUE(args);
      if (value_node == NULL_TREE) {
         context.report_error("the default value cannot be blank");
      }
      
      auto vt = context.bitpacking_value_type();
      if (vt.is_boolean() || vt.is_enum() || vt.is_integer()) {
         _check_and_report_integral_value(context, value_node);
      } else if (vt.is_pointer()) {
         _check_and_report_pointer_value(context, value_node);
      } else if (vt.is_floating_point()) {
         _check_and_report_real_value(context, value_node);
      } else if (vt.is_array()) { // will only be true for strings
         _check_and_report_string_value(context, value_node);
      } else {
         context.report_error("this attribute cannot be applied to this type");
      }
      if (context.has_any_errors()) {
         *no_add_attrs = true;
      }
      return NULL_TREE;
   }
}