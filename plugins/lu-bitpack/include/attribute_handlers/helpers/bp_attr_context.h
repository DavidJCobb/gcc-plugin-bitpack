#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "lu/strings/zview.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/list_node.h"
#include <diagnostic.h>

namespace attribute_handlers::helpers {
   struct bp_attr_context {
      public:
         bp_attr_context(tree* node_ptr, tree attribute_name, int flags);
      
      protected:
         tree  attribute_name;
         tree* attribute_target = nullptr;
         int   attribute_flags  = 0;
         
         bool failed = false;
         
         std::string printable; // lazy-created and cached
         
      protected:
         void _report_applied_to_impossible_type(tree type);
      
         void _report_type_mismatch(
            lu::strings::zview meant_for,
            lu::strings::zview applied_to
         );
         
         const std::string& _get_context_string();
         
         void _mark_with_error_attribute();
         
         gcc_wrappers::type::base type_of_target() const;
         
      public:
         constexpr bool has_any_errors() const noexcept {
            return this->failed;
         }
      
         bool check_and_report_applied_to_integral();
         bool check_and_report_applied_to_string();
         
         // Generally, returns the innermost value type. If the target type is 
         // a string or array thereof, returns an array of the character type.
         gcc_wrappers::type::base bitpacking_value_type() const;
         
         bool bitpacking_value_type_is_string() const;
         
         void add_internal_attribute(lu::strings::zview attr_name, gcc_wrappers::list_node = {});
         
         template<typename... Args>
         void report_error(std::string_view text, Args&&... args) {
            std::string message = "%qE attribute ";
            if (const auto& ctxt = _get_context_string(); !ctxt.empty()) {
               message += ctxt;
               message += ' ';
            }
            message += text;
            error(message.c_str(), this->attribute_name, std::forward<Args>(args)...);
            this->_mark_with_error_attribute();
         }
   };
}