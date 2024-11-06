#include "attribute_handlers/bitpack_range.h"
#include <cinttypes> // PRIdMAX and friends
#include <cstdint>
#include "lu/strings/printf_string.h"
#include "attribute_handlers/generic_type_or_decl.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/list_node.h"
#include <diagnostic.h>
#include <print-tree.h>

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
      
      std::string context;
      //
      bool type_is_invalid = false;
      
      gw::type::base value_type;
      if (node_ptr) {
         auto node = *node_ptr;
         
         gw::type::base type;
         if (TYPE_P(node)) {
            type = gw::type::base::from_untyped(node);
         } else if (TREE_CODE(node) == TYPE_DECL) {
            type = gw::type::base::from_untyped(TREE_TYPE(node));
         } else if (TREE_CODE(node) == FIELD_DECL) {
            value_type = gw::type::base::from_untyped(TREE_TYPE(node));
            context    = gw::decl::field::from_untyped(node).name();
            context    = lu::strings::printf_string(" (applied to field %<%s%>)", context.c_str());
         }
         if (!type.empty()) {
            value_type = type;
            context    = type.pretty_print();
            context    = lu::strings::printf_string(" (applied to type %<%s%>)", context.c_str());
         }
      }
      if (!value_type.empty()) {
         auto _report_type_mismatch = [name, &context](const char* what) {
            std::string message = "%qE attribute";
            if (!context.empty())
               message += context;
            message += " is meant for integer types; you are applying it to %s";
            error(message.c_str(), name, what);
         };
         
         type_is_invalid = true;
         if (value_type.is_pointer()) {
            _report_type_mismatch("a pointer");
         } else if (value_type.is_record()) {
            _report_type_mismatch("a struct");
         } else if (value_type.is_union()) {
            _report_type_mismatch("a union");
         } else if (value_type.is_void()) {
            _report_type_mismatch("void");
         } else {
            type_is_invalid = false;
         }
         if (type_is_invalid) {
            //
            // Offer some helpful advice about a future edge-case.
            //
            inform(UNKNOWN_LOCATION, "if you are using transform functions to pack this object as an integer, then apply this attribute to the transformed (integer) type instead. if that type is a built-in e.g. %<uint8_t%>, then use a typedef of it");
         }
      }
      
      std::optional<intmax_t> min;
      std::optional<intmax_t> max;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && TREE_CODE(next) == INTEGER_CST) {
            min = gw::expr::integer_constant::from_untyped(next).try_value_signed();
         }
         
         args = TREE_CHAIN(args);
         if (args != NULL_TREE) {
            auto next = TREE_VALUE(args);
            if (next != NULL_TREE && TREE_CODE(next) == INTEGER_CST) {
               max = gw::expr::integer_constant::from_untyped(next).try_value_signed();
            }
         }
      }
      if (!min.has_value() || !max.has_value()) {
         auto message = std::string("%qE attribute") + context + " %s argument (%s value) must be an integer constant";
         if (!min.has_value()) {
            error(message.c_str(), name, "first", "minimum");
         }
         if (!max.has_value()) {
            error(message.c_str(), name, "second", "maximum");
         }
         goto failure;
      }
      if (*min > *max) {
         auto message = std::string("%qE attribute") + context + " minimum value (first argument) is greater than the maximum value (second argument) i.e. %" PRIdMAX " is greater than %" PRIdMAX "";
         error(message.c_str(), name, *min, *max);
         goto failure;
      }
      if (type_is_invalid) {
         goto failure;
      }
      
      *no_add_attrs = false;
      return NULL_TREE;
      
   failure:
      //*no_add_attrs = true; // actually, do add the argument, so codegen can fail later on
      return NULL_TREE;
   }
}