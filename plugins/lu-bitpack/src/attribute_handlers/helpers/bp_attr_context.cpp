#include "attribute_handlers/helpers/bp_attr_context.h"
#include "lu/strings/printf_string.h"
#include <stringpool.h> // dependency of <attribs.h>
#include <attribs.h>
#include "gcc_wrappers/type/array.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers::helpers {
   bp_attr_context::bp_attr_context(tree attribute_name, tree node) {
      this->attribute_name = attribute_name;
      if (node == NULL_TREE) {
         return;
      }
      
      if (TYPE_P(node)) {
         this->type = gw::type::base::from_untyped(node);
      } else if (DECL_P(node)) {
         if (TREE_CODE(node) == TYPE_DECL) {
            this->type = gw::type::base::from_untyped(TREE_TYPE(node));
         } else if (TREE_CODE(node) == FIELD_DECL) {
            this->decl = gw::decl::field::from_untyped(node);
            this->type = this->decl.value_type();
         } else {
            report_error("cannot be applied to this kind of declaration; use it on types or fields");
            return;
         }
      }
      
      if (!this->type.empty()) {
         if (this->decl.empty()) {
            //
            // We're being applied to a type. Validate it.
            //
            if (this->type.is_array() || this->type.is_function()) {
               _report_applied_to_impossible_type(this->type.as_untyped());
               this->type = {};
               return;
            }
         }
         //
         // Compute the value type.
         //
         this->value_type = this->type;
         while (this->value_type.is_array()) {
            this->value_type = this->value_type.as_array().value_type();
         }
      }
   }
   
   void bp_attr_context::_report_applied_to_impossible_type(tree type) const {
      constexpr const char* const sentinel_name = "lu bitpack already reported application to bad type";
      
      assert(type != NULL_TREE && TYPE_P(type));
      auto attr = lookup_attribute(sentinel_name, type);
      if (attr != NULL_TREE)
         return;
      
      attr = tree_cons(get_identifier(sentinel_name), NULL_TREE, NULL_TREE);
      decl_attributes(&type, attr, ATTR_FLAG_TYPE_IN_PLACE | ATTR_FLAG_INTERNAL, nullptr);
      
      report_error("cannot be applied directly to array or function types");
   }
   
   void bp_attr_context::_report_type_mismatch(
      lu::strings::zview meant_for,
      lu::strings::zview applied_to
   ) const {
      report_error(
         "is meant for %s; you are appling it to %s",
         meant_for.c_str(),
         applied_to.c_str()
      );
   }
   
   std::string& bp_attr_context::_get_context_string() const {
      auto& str = this->printable;
      if (str.empty()) {
         if (!this->decl.empty()) {
            str = lu::strings::printf_string("(applied to field %<%s%>)", this->decl.name().data());
         } else if (!this->type.empty()) {
            str = lu::strings::printf_string("(applied to type %<%s%>)", this->type.pretty_print());
         }
      }
      return str;
   }
   
   bool bp_attr_context::check_and_report_applied_to_integral() const {
      if (this->value_type.empty())
         return true;
      
      constexpr const char* const transform_tip = "if you intend to use bitpack transformation functions to pack this object as an integer, then apply this attribute to the transformed (integer) type instead. if that type is a built-in e.g. %<uint8_t%>, then use a typedef of it";
      
      if (this->value_type.is_pointer()) {
         _report_type_mismatch("integer types", "a pointer");
         inform(UNKNOWN_LOCATION, transform_tip);
         return false;
      }
      if (this->value_type.is_record()) {
         _report_type_mismatch("integer types", "a struct");
         inform(UNKNOWN_LOCATION, transform_tip);
         return false;
      }
      if (this->value_type.is_union()) {
         _report_type_mismatch("integer types", "a union");
         inform(UNKNOWN_LOCATION, transform_tip);
         return false;
      }
      if (this->value_type.is_void()) {
         _report_type_mismatch("integer types", "void");
         inform(UNKNOWN_LOCATION, transform_tip);
         return false;
      }
      
      return true;
   }
   
   std::optional<bitpacking::data_options::processed_bitpack_attributes> bp_attr_context::get_destination() const {
      using dst_type = bitpacking::data_options::processed_bitpack_attributes;
      
      if (!this->decl.empty()) {
         return dst_type(this->decl);
      }
      if (!this->value_type.empty()) {
         return dst_type(this->value_type);
      }
      
      return {};
   }
}