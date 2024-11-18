#include "attribute_handlers/helpers/bp_attr_context.h"
#include <array>
#include <utility> // std::pair
#include "lu/strings/printf_string.h"
#include <stringpool.h> // dependency of <attribs.h>
#include <attribs.h>
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/type/array.h"
#include "basic_global_state.h"
#include "debugprint.h"
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
            case VAR_DECL:
               break;
            default:
               report_error("cannot be applied to this kind of declaration; use it on types, variables, or struct/union fields");
               break;
         }
      }
   }
   
   const std::string& bp_attr_context::_get_context_string() {
      auto& str = this->printable;
      if (str.empty()) {
         auto node = this->attribute_target[0];
         
         gw::type::base type;
         gw::decl::base decl;
         if (TYPE_P(node)) {
            type = gw::type::base::from_untyped(node);
         } else if (DECL_P(node)) {
            decl = gw::decl::base::from_untyped(node);
            type = gw::type::base::from_untyped(TREE_TYPE(node));
         }
         
         if (!decl.empty()) {
            if (TREE_CODE(node) == TYPE_DECL) {
               str = lu::strings::printf_string("(applied to type %<%s%>)", decl.name().data());
            } else {
               str = lu::strings::printf_string("(applied to field %<%s%>)", decl.name().data());
            }
         } else if (!type.empty()) {
            auto pp = type.pretty_print();
            str = lu::strings::printf_string("(applied to type %<%s%>)", pp.c_str());
         }
      }
      return str;
   }
   
   void bp_attr_context::_mark_with_named_error_attribute() {
      constexpr const char* const sentinel_name = "lu bitpack invalid attribute name";
      
      bool here = false;
      auto list = get_existing_attributes();
      for(auto attr : list) {
         if (attr.name() != sentinel_name)
            continue;
         auto name = attr.arguments().front();
         if (name.code() != IDENTIFIER_NODE)
            continue;
         if (name.as_untyped() == this->attribute_name) {
            here = true;
            break;
         }
      }
      if (here)
         return;
      
      auto attr_args = tree_cons(NULL_TREE, this->attribute_name, NULL_TREE);
      auto attr_node = tree_cons(get_identifier(sentinel_name), attr_args, NULL_TREE);
      auto deferred  = decl_attributes(
         this->attribute_target,
         attr_node,
         this->attribute_flags | ATTR_FLAG_INTERNAL,
         this->attribute_target[1]
      );
      assert(deferred == NULL_TREE);
   }
   void bp_attr_context::_mark_with_error_attribute() {
      constexpr const char* const sentinel_name = "lu bitpack invalid attributes";
      
      this->failed = true;
      
      if (get_existing_attributes().has_attribute(sentinel_name))
         return;
      
      auto attr_node = tree_cons(get_identifier(sentinel_name), NULL_TREE, NULL_TREE);
      auto deferred  = decl_attributes(
         this->attribute_target,
         attr_node,
         this->attribute_flags | ATTR_FLAG_INTERNAL,
         this->attribute_target[1]
      );
      assert(deferred == NULL_TREE);
      assert(get_existing_attributes().has_attribute(sentinel_name));
   }
   
   bool bp_attr_context::attribute_already_applied() const {
      return get_existing_attributes().has_attribute(this->attribute_name);
   }
   bool bp_attr_context::attribute_already_attempted() const {
      auto list = get_existing_attributes();
      for(auto attr : list) {
         auto name = attr.name();
         if (name == IDENTIFIER_POINTER(this->attribute_name))
            return true;
         if (name != "lu bitpack invalid attribute name")
            continue;
         auto id_node = attr.arguments().front().as_untyped();
         assert(id_node != NULL_TREE && TREE_CODE(id_node) == IDENTIFIER_NODE);
         if (id_node == this->attribute_name)
            return true;
      }
      return false;
   }
   
   gw::attribute_list bp_attr_context::get_existing_attributes() const {
      auto node = this->attribute_target[0];
      if (node == NULL_TREE)
         return {};
      if (DECL_P(node))
         return gw::attribute_list::from_untyped(DECL_ATTRIBUTES(node));
      if (TYPE_P(node))
         return gw::attribute_list::from_untyped(TYPE_ATTRIBUTES(node));
      return {};
   }
   
   void bp_attr_context::check_and_report_applied_to_integral() {
      auto type = type_of_target();
      assert(!type.empty());
      bool   is_array = type.is_array();
      size_t rank     = 0;
      while (type.is_array()) {
         ++rank;
         type = type.as_array().value_type();
      }
      if (type.is_boolean() || type.is_enum() || type.is_integer()) {
         return;
      }
      auto pp = type.pretty_print();
      if (is_array) {
         if (rank > 1) {
            report_error("applied to multi-dimensional array of a non-integral type %qs", pp.c_str());
         } else {
            report_error("applied to an array of non-integral type %qs", pp.c_str());
         }
      } else {
         report_error("applied to non-integral type %qs", pp.c_str());
      }
   }
   
   bool bp_attr_context::_impl_check_x_options(tree subject, x_option_type here) {
      constexpr const auto attribs_buffer = std::array{
         "lu_bitpack_as_opaque_buffer",
      };
      constexpr const auto attribs_integral = std::array{
         "lu_bitpack_bitcount",
         "lu_bitpack_range",
      };
      constexpr const auto attribs_string = std::array{
         "lu_bitpack_string",
      };
      constexpr const auto attribs_transforms = std::array{
         "lu_bitpack_transforms",
      };
      
      gw::attribute_list list;
      if (TYPE_P(subject))
         list = gw::type::base::from_untyped(subject).attributes();
      else
         list = gw::decl::base::from_untyped(subject).attributes();
      
      auto _has = [&list](std::string_view name) {
         for(auto attr : list) {
            if (attr.name() == name)
               return true;
            if (attr.name() == "lu bitpack invalid attribute name") {
               auto id_node = attr.arguments().front().as_untyped();
               assert(id_node != NULL_TREE && TREE_CODE(id_node) == IDENTIFIER_NODE);
               if (IDENTIFIER_POINTER(id_node) == name)
                  return true;
            }
         }
         return false;
      };
      
      if (here != x_option_type::buffer)
         for(const char* name : attribs_buffer)
            if (_has(name))
               return true;
      if (here != x_option_type::integral)
         for(const char* name : attribs_integral)
            if (_has(name))
               return true;
      if (here != x_option_type::string)
         for(const char* name : attribs_string)
            if (_has(name))
               return true;
      if (here != x_option_type::transforms)
         for(const char* name : attribs_transforms)
            if (_has(name))
               return true;
            
      return false;
   }
   void bp_attr_context::check_and_report_contradictory_x_options(x_option_type here) {
      auto type = target_type();
      if (type.empty()) {
         if (_impl_check_x_options(this->attribute_target[0], here)) {
            report_error("conflicts with attributes previously applied to this field");
            return;
         }
         type = type_of_target();
      }
      if (!type.empty()) {
         if (_impl_check_x_options(type.as_untyped(), here)) {
            report_error("conflicts with attributes previously applied to this type");
            return;
         }
         while (type.is_array()) {
            type = type.as_array().value_type();
            if (_impl_check_x_options(type.as_untyped(), here)) {
               report_error("conflicts with attributes previously applied to this array type or its value type(s)");
               return;
            }
         }
         return;
      }
   }
   
   gw::decl::field bp_attr_context::target_field() const {
      tree node = this->attribute_target[0];
      if (TREE_CODE(node) == FIELD_DECL)
         return gw::decl::field::from_untyped(node);
      return {};
   }
   gw::decl::type_def bp_attr_context::target_typedef() const {
      tree node = this->attribute_target[0];
      if (TREE_CODE(node) == TYPE_DECL)
         return gw::decl::type_def::from_untyped(node);
      return {};
   }
   gw::type::base bp_attr_context::target_type() const {
      tree node = this->attribute_target[0];
      if (TYPE_P(node))
         return gw::type::base::from_untyped(node);
      return {};
   }
   
   gcc_wrappers::type::base bp_attr_context::type_of_target() const {
      auto node = this->attribute_target[0];
      if (TYPE_P(node))
         return gw::type::base::from_untyped(node);
      assert(DECL_P(node));
      
      gw::type::base type;
      switch (TREE_CODE(node)) {
         case TYPE_DECL:
            //
            // When attribute handlers are being invoked on a TYPE_DECL, 
            // DECL_ORIGINAL_TYPE hasn't been set yet; the original type 
            // is instead in TREE_TYPE.
            //
            type = gw::type::base::from_untyped(TREE_TYPE(node));
            break;
         case FIELD_DECL:
            type = gw::decl::field::from_untyped(node).value_type();
            break;
         case VAR_DECL:
            type = gw::decl::variable::from_untyped(node).value_type();
            break;
         default:
            break;
      }
      assert(!type.empty());
      return type;
   }
   
   void bp_attr_context::reapply_with_new_args(gcc_wrappers::list_node args) {
      auto attr_node = tree_cons(this->attribute_name, args.as_untyped(), NULL_TREE);
      decl_attributes(
         this->attribute_target,
         attr_node,
         this->attribute_flags | ATTR_FLAG_INTERNAL,
         this->attribute_target[1]
      );
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
   
   void bp_attr_context::fail_silently() {
      this->_mark_with_error_attribute();
      this->_mark_with_named_error_attribute();
   }
}