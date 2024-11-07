#include "attribute_handlers/helpers/bp_attr_context.h"
#include "lu/strings/printf_string.h"
#include <stringpool.h> // dependency of <attribs.h>
#include <attribs.h>
#include "gcc_wrappers/type/array.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers::helpers {
   bp_attr_context::bp_attr_context(tree* node_ptr, tree attribute_name, int flags) {
      this->attribute_target = node_ptr;
      this->attribute_name   = attribute_name;
      this->attribute_flags  = flags;
      tree node = *node_ptr;
      
      if (node == NULL_TREE) {
         return;
      }
      
      gw::type::base type;
      if (TYPE_P(node)) {
         type = gw::type::base::from_untyped(node);
      } else if (DECL_P(node)) {
         switch (TREE_CODE(node)) {
            case TYPE_DECL:
               type = gw::type::base::from_untyped(TREE_TYPE(node));
               break;
            case FIELD_DECL:
               break;
            default:
               report_error("cannot be applied to this kind of declaration; use it on types or fields");
               break;
         }
      }
      
      if (!type.empty()) {
         if (!DECL_P(node) || TREE_CODE(node) != TYPE_DECL) {
            //
            // We're being applied to a type. Validate it.
            //
            if (type.is_array() || type.is_function()) {
               _report_applied_to_impossible_type(type.as_untyped());
               return;
            }
         }
      }
   }
   
   void bp_attr_context::_report_applied_to_impossible_type(tree type) {
      constexpr const char* const sentinel_name = "lu bitpack already reported application to bad type";
      
      assert(type != NULL_TREE && TYPE_P(type));
      auto attr = lookup_attribute(sentinel_name, TYPE_ATTRIBUTES(type));
      if (attr != NULL_TREE)
         return;
      
      attr = tree_cons(get_identifier(sentinel_name), NULL_TREE, NULL_TREE);
      decl_attributes(this->attribute_target, attr, this->attribute_flags | ATTR_FLAG_INTERNAL, nullptr);
      
      report_error("cannot be applied directly to array or function types");
   }
   
   void bp_attr_context::_report_type_mismatch(
      lu::strings::zview meant_for,
      lu::strings::zview applied_to
   ) {
      report_error(
         "is meant for %s; you are appling it to %s",
         meant_for.c_str(),
         applied_to.c_str()
      );
   }
   
   const std::string& bp_attr_context::_get_context_string() {
      auto& str = this->printable;
      if (str.empty()) {
         auto node = this->attribute_target[0];
         
         gw::type::base  type;
         gw::decl::field decl;
         if (TYPE_P(node)) {
            type = gw::type::base::from_untyped(node);
         } else if (DECL_P(node)) {
            if (TREE_CODE(node) == TYPE_DECL) {
               type = gw::type::base::from_untyped(TREE_TYPE(node));
            } else if (TREE_CODE(node) == FIELD_DECL) {
               decl = gw::decl::field::from_untyped(node);
               type = decl.value_type();
            }
         }
         
         if (!decl.empty()) {
            str = lu::strings::printf_string("(applied to field %<%s%>)", decl.name().data());
         } else if (!type.empty()) {
            str = lu::strings::printf_string("(applied to type %<%s%>)", type.pretty_print());
         }
      }
      return str;
   }
         
   void bp_attr_context::_mark_with_error_attribute() {
      constexpr const char* const sentinel_name = "lu bitpack invalid attributes";
      
      this->failed = true;
      
      {
         tree list = NULL_TREE;
         tree node = this->attribute_target[0];
         if (TYPE_P(node)) {
            list = TYPE_ATTRIBUTES(node);
         } else {
            list = DECL_ATTRIBUTES(node);
         }
         auto attr = lookup_attribute(sentinel_name, list);
         if (attr != NULL_TREE)
            return;
      }
      
      auto attr_node = tree_cons(get_identifier(sentinel_name), NULL_TREE, NULL_TREE);
      auto deferred  = decl_attributes(
         this->attribute_target,
         attr_node,
         this->attribute_flags | ATTR_FLAG_INTERNAL,
         this->attribute_target[1]
      );
      assert(deferred == NULL_TREE);
   }
   
   gw::type::base bp_attr_context::type_of_target() const {
      tree node = this->attribute_target[0];
      if (node == NULL_TREE)
         return {};
      
      if (TYPE_P(node))
         return gw::type::base::from_untyped(node);
      if (TREE_CODE(node) == TYPE_DECL)
         return gw::type::base::from_untyped(TREE_TYPE(node));
      
      if (!DECL_P(node))
         return {};
      
      auto type = gw::type::base::from_untyped(TREE_TYPE(node));
      while (type.is_array()) {
         type = type.as_array().value_type();
      }
      return type;
   }
   
   bool bp_attr_context::check_and_report_applied_to_integral() {
      auto type = this->type_of_target();
      if (type.empty())
         return true;
      
      constexpr const char* const transform_tip = "if you intend to use bitpack transformation functions to pack this object as an integer, then apply this attribute to the transformed (integer) type instead. if that type is a built-in e.g. %<uint8_t%>, then use a typedef of it";
      
      if (type.is_pointer()) {
         _report_type_mismatch("integer types", "a pointer");
         inform(UNKNOWN_LOCATION, transform_tip);
         return false;
      }
      if (type.is_record()) {
         _report_type_mismatch("integer types", "a struct");
         inform(UNKNOWN_LOCATION, transform_tip);
         return false;
      }
      if (type.is_union()) {
         _report_type_mismatch("integer types", "a union");
         inform(UNKNOWN_LOCATION, transform_tip);
         return false;
      }
      if (type.is_void()) {
         _report_type_mismatch("integer types", "void");
         inform(UNKNOWN_LOCATION, transform_tip);
         return false;
      }
      
      return true;
   }
   
   void bp_attr_context::add_internal_attribute(lu::strings::zview attr_name, gw::list_node args) {
      auto attr_node = tree_cons(get_identifier(attr_name.c_str()), args.as_untyped(), NULL_TREE);
      decl_attributes(
         this->attribute_target,
         attr_node,
         this->attribute_flags | ATTR_FLAG_INTERNAL,
         this->attribute_target[1]
      );
   }
}