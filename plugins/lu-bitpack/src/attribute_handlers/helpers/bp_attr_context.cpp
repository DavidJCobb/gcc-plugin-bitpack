#include "attribute_handlers/helpers/bp_attr_context.h"
#include <array>
#include <utility> // std::pair
#include "lu/strings/printf_string.h"
#include <stringpool.h> // dependency of <attribs.h>
#include <attribs.h>
#include "gcc_wrappers/type/array.h"
#include "basic_global_state.h"
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
   bool bp_attr_context::check_and_report_applied_to_string() {
      gw::type::base type;
      {
         auto node = this->attribute_target[0];
         if (node == NULL_TREE)
            return true;
         if (TYPE_P(node))
            type.set_from_untyped(node);
         else if (TREE_CODE(node) == TYPE_DECL)
            type.set_from_untyped(TREE_TYPE(node));
         else if (DECL_P(node))
            type.set_from_untyped(TREE_TYPE(node));
         else
            return true;
      }
      if (type.empty())
         return true;
      
      auto& global       = basic_global_state::get().global_options.computed;
      auto  bitpack_type = bitpacking_value_type();
      if (global.type_is_string(bitpack_type))
         return true;
      
      //
      // Report error information:
      //
      
      bool is_array = type.is_array();
      while (type.is_array()) {
         type = type.as_array().value_type();
      }
      
      constexpr const auto nouns = std::array{
         std::pair{ "a non-string", "an array of non-strings" },
         std::pair{ "a boolean",    "an array of booleans" },
         std::pair{ "an enum",      "an array of enums" },
         std::pair{ "a float",      "an array of floats" },
         std::pair{ "an integer",   "an array of integers" },
         std::pair{ "a pointer",    "an array of pointers" },
         std::pair{ "a struct",     "an array of structs" },
         std::pair{ "a union",      "an array of unions" },
         std::pair{ "void",         "an array of void" },
      };
      enum class noun_type {
         unknown,
         boolean,
         enumeration,
         floating_point,
         integer,
         pointer,
         record,
         union_type,
         void_type,
      };
      
      std::string what;
      {
         size_t i = 0;
         if (type.is_boolean())
            i = (size_t)noun_type::boolean;
         else if (type.is_enum())
            i = (size_t)noun_type::enumeration;
         else if (type.is_floating_point())
            i = (size_t)noun_type::floating_point;
         else if (type.is_integer())
            i = (size_t)noun_type::integer;
         else if (type.is_pointer())
            i = (size_t)noun_type::pointer;
         else if (type.is_record())
            i = (size_t)noun_type::record;
         else if (type.is_union())
            i = (size_t)noun_type::union_type;
         else if (type.is_void())
            i = (size_t)noun_type::void_type;
         
         if (is_array) {
            what = nouns[i].second;
         } else {
            what = nouns[i].first;
         }
      }
      _report_type_mismatch("string types", what);
      if (is_array && type.is_integer()) {
         auto pp = global.types.string_char.pretty_print();
         inform(UNKNOWN_LOCATION, "the global bitpacking options specify %<%s%> as the character type for bitpacking; a \"string\" is thus defined as an array of %<%s%>", pp.c_str(), pp.c_str());
      }
      return false;
   }
   
   gw::type::base bp_attr_context::bitpacking_value_type() const {
      tree node = this->attribute_target[0];
      if (node == NULL_TREE)
         return {};
      
      gw::type::base type;
      if (TYPE_P(node))
         type.set_from_untyped(node);
      else {
         switch (TREE_CODE(node)) {
            case FIELD_DECL:
            case TYPE_DECL:
            case VAR_DECL:
               type.set_from_untyped(TREE_TYPE(node));
               break;
            default:
               break;
         }
      }
      if (type.empty())
         return {};
      
      return basic_global_state::get().global_options.computed.innermost_value_type(type);
   }
   
   bool bp_attr_context::bitpacking_value_type_is_string() const {
      auto& global = basic_global_state::get().global_options.computed;
      return global.type_is_string(bitpacking_value_type());
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
}