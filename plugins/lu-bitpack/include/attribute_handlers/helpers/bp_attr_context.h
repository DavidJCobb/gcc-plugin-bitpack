#pragma once
#include <optional>
#include <string>
#include <string_view>
#include "lu/strings/zview.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/type/base.h"
#include <diagnostic.h>
#include "bitpacking/data_options/processed_bitpack_attributes.h"

namespace attribute_handlers::helpers {
   struct bp_attr_context {
      public:
         bp_attr_context(tree attribute_name, tree target);
      
      protected:
         tree attribute_name;
      
         gcc_wrappers::decl::field decl;
         gcc_wrappers::type::base  type;
         gcc_wrappers::type::base  value_type;
         mutable std::string       printable; // lazy-created and cached
         
      protected:
         void _report_applied_to_impossible_type(tree type) const;
      
         void _report_type_mismatch(
            lu::strings::zview meant_for,
            lu::strings::zview applied_to
         ) const;
         
         std::string& _get_context_string() const;
         
      public:
         bool check_and_report_applied_to_integral() const;
         
         template<typename... Args>
         void report_error(std::string_view text, Args&&... args) const {
            std::string message = "%qE attribute ";
            if (const auto& ctxt = _get_context_string(); !ctxt.empty()) {
               message += ctxt;
               message += ' ';
            }
            message += text;
            error(message.c_str(), this->attribute_name, std::forward<Args>(args)...);
         }
         
         std::optional<bitpacking::data_options::processed_bitpack_attributes> get_destination() const;
   };
}