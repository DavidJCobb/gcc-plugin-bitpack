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
#include "gcc_wrappers/attribute.h"
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
         const std::string& _get_context_string();
         
         // The error-and-name attribute exists so that attributes can quickly 
         // check whether they're being applied more than once to an entity, 
         // counting previous failed application attempts.
         void _mark_with_named_error_attribute();
         
         // The general-purpose error attribute exists so that our codegen and 
         // other systems can quickly check whether any attribute on an entity 
         // was invalid.
         void _mark_with_error_attribute();
         
         void _report_applied_to_impossible_type(tree type);
      
         void _report_type_mismatch(
            lu::strings::zview meant_for,
            lu::strings::zview applied_to
         );
         
         gcc_wrappers::type::base type_of_target() const;
         
      public:
         constexpr bool has_any_errors() const noexcept {
            return this->failed;
         }
         
         bool attribute_already_applied() const;
         bool attribute_already_attempted() const; // includes failures
         
         // May be an empty wrapper.
         gcc_wrappers::attribute_list get_existing_attributes() const;
      
         bool check_and_report_applied_to_integral();
         bool check_and_report_applied_to_string();
         
         // Generally, returns the innermost value type. If the target type is 
         // a string or array thereof, returns an array of the character type.
         gcc_wrappers::type::base bitpacking_value_type() const;
         
         // Check whether `bitpacking_value_type` is an array of the string 
         // character type specified in the global bitpacking options.
         bool bitpacking_value_type_is_string() const;
         
         // Apply a copy of the current attribute, with the specified args, to 
         // the current attribute target. An attribute handler can use this to 
         // quickly mutate the arguments to be applied, though it will need to 
         // set `*no_add_attrs = true` to prevent the un-mutated version from 
         // being applied.
         //
         // The attribute handler will be invoked with `ATTR_FLAG_INTERNAL`, 
         // so it should check for that flag and early-out without any further 
         // validation when that flag is present.
         void reapply_with_new_args(gcc_wrappers::list_node);
         
         void add_internal_attribute(lu::strings::zview attr_name, gcc_wrappers::list_node = {});
         
         // Report an error, and apply sentinel attributes to the target so that 
         // we can check if it has any failed bitpacking attributes.
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
            this->_mark_with_named_error_attribute();
         }
         
         // GCC inform
         template<typename... Args>
         void report_note(lu::strings::zview text, Args&&... args) {
            inform(UNKNOWN_LOCATION, text.c_str(), std::forward<Args>(args)...);
         }
   };
}