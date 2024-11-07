#include "bitpacking/data_options/processed_bitpack_attributes.h"
#include <cassert>
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/builtin_types.h"
#include <stringpool.h> // get_identifier; dependency of <attribs.h>
#include <attribs.h>
namespace gw {
   using namespace gcc_wrappers;
}
template<typename T>
using specified = bitpacking::data_options::processed_bitpack_attributes::specified<T>;

#include <diagnostic.h> // debug

namespace bitpacking::data_options {
   processed_bitpack_attributes::processed_bitpack_attributes(gw::decl::base decl) {
      assert(!decl.empty());
      this->_node = decl.as_untyped();
   }
   processed_bitpack_attributes::processed_bitpack_attributes(gw::type::base type) {
      assert(!type.empty());
      this->_node = type.as_untyped();
   }
   
   gw::attribute processed_bitpack_attributes::_get_attribute() const {
      gw::attribute_list list;
      if (TYPE_P(this->_node)) {
         list = gw::type::base::from_untyped(this->_node).attributes();
      } else {
         assert(DECL_P(this->_node));
         list = gw::decl::base::from_untyped(this->_node).attributes();
      }
      auto attr = list.get_attribute("lu bitpack computed");
      return attr;
   }
   gw::attribute processed_bitpack_attributes::_get_or_create_attribute() {
      auto attr = _get_attribute();
      if (!attr.empty())
         return attr;
      
      auto args = tree_cons(NULL_TREE, NULL_TREE, NULL_TREE);
      
      int flags = ATTR_FLAG_INTERNAL;
      if (TYPE_P(this->_node))
         flags |= ATTR_FLAG_TYPE_IN_PLACE;
      
      auto attr_node = tree_cons(get_identifier("lu bitpack computed"), args, NULL_TREE);
      auto deferred  = decl_attributes(&this->_node, attr_node, flags, nullptr);
      assert(deferred == NULL_TREE);
      
      return gw::attribute::from_untyped(attr_node);
   }
   
   tree processed_bitpack_attributes::_get_map(gw::attribute attr) const {
      tree args = TREE_VALUE(attr.as_untyped());
      assert(args != NULL_TREE);
      tree map  = TREE_VALUE(args); // first argument
      return map;
   }
   
   tree processed_bitpack_attributes::_get_value_node(std::string_view key) const {
      auto attr = _get_attribute();
      if (attr.empty())
         return NULL_TREE;
      auto map = _get_map(attr);
      for(auto node = map; node != NULL_TREE; node = TREE_CHAIN(node)) {
         auto node_k = TREE_PURPOSE(node);
         auto node_v = TREE_VALUE(node);
         assert(node_k != NULL_TREE && TREE_CODE(node_k) == IDENTIFIER_NODE);
         
         if (std::string_view(IDENTIFIER_POINTER(node_k)) == key) {
            return node_v;
         }
      }
      return NULL_TREE;
   }
   tree processed_bitpack_attributes::_get_or_create_map_pair(lu::strings::zview key) {
      auto attr = _get_or_create_attribute();
      assert(!attr.empty());
      auto map  = _get_map(attr);
      tree pair = map;
      tree back = NULL_TREE;
      for(; pair != NULL_TREE; back = pair, pair = TREE_CHAIN(pair)) {
         auto node_k = TREE_PURPOSE(pair);
         assert(node_k != NULL_TREE && TREE_CODE(node_k) == IDENTIFIER_NODE);
         
         if (lu::strings::zview(IDENTIFIER_POINTER(node_k)) == key) {
            return pair;
         }
      }
      
      pair = tree_cons(get_identifier(key.c_str()), NULL_TREE, NULL_TREE);
      if (back == NULL_TREE) {
         TREE_VALUE(TREE_VALUE(attr.as_untyped())) = pair;
      } else {
         TREE_CHAIN(back) = pair;
      }
      return pair;
   }
   
   /*static*/ bool _compare_nodes(tree a, tree b) { 
      if (a == b)
         return true;
      if (a == NULL_TREE)
         return false;
      
      auto code = TREE_CODE(a);
      if (code != TREE_CODE(b))
         return false;
      
      switch (code) {
         case INTEGER_CST:
            {
               auto wrap_a = gw::expr::integer_constant::from_untyped(a);
               auto wrap_b = gw::expr::integer_constant::from_untyped(b);
               return wrap_a == wrap_b;
            }
            break;
         case STRING_CST:
            {
               auto wrap_a = gw::expr::string_constant::from_untyped(a);
               auto wrap_b = gw::expr::string_constant::from_untyped(b);
               return wrap_a.value_view() == wrap_b.value_view();
            }
            break;
         default:
            break;
      }
      
      return false;
   }
   
   tree processed_bitpack_attributes::get_raw_property(std::string_view name) const {
      return _get_value_node(name);
   }
   void processed_bitpack_attributes::set_raw_property(lu::strings::zview name, tree node) {
      if (node == NULL_TREE && get_raw_property(name) == NULL_TREE)
         return;
      
      auto pair  = _get_or_create_map_pair(name);
      auto value = TREE_VALUE(pair);
      if (_compare_nodes(value, node))
         return;
      
      TREE_VALUE(pair) = node;
   }
   
   //
   // Private templates and related accessors:
   //
   
   template<std::integral Integral>
   specified<Integral> processed_bitpack_attributes::_get_integral_property(std::string_view name) const {
      auto node = get_raw_property(name);
      if (node == NULL_TREE)
         return {};
      if (node == error_mark_node)
         return invalid_tag{};
      
      assert(TREE_CODE(node) == INTEGER_CST);
      auto opt = gw::expr::integer_constant::from_untyped(node).value<Integral>();
      if (!opt.has_value())
         return invalid_tag{};
      return *opt;
   }
         
   template<typename Integral>
   void processed_bitpack_attributes::_set_integral_property(lu::strings::zview name, specified<Integral> value) requires gw::builtin_types::integer_type_ready_for<Integral> {
      tree node;
      if (std::holds_alternative<std::monostate>(value)) {
         node = NULL_TREE;
      } else if (std::holds_alternative<invalid_tag>(value)) {
         node = error_mark_node;
      } else {
         node = gw::expr::integer_constant(
            gw::builtin_types::get().type_for<Integral>(),
            std::get<Integral>(value)
         ).as_untyped();
      }
      set_raw_property(name, node);
   }
   
   specified<std::string> processed_bitpack_attributes::_get_identifier_property(std::string_view name) const {
      auto node = get_raw_property(name);
      if (node == NULL_TREE)
         return {};
      if (node == error_mark_node)
         return invalid_tag{};
      
      assert(TREE_CODE(node) == IDENTIFIER_NODE);
      return std::string(IDENTIFIER_POINTER(node));
   }
   void processed_bitpack_attributes::_set_identifier_property(lu::strings::zview name, specified<std::string> value) {
      tree node;
      if (std::holds_alternative<std::monostate>(value)) {
         node = NULL_TREE;
      } else if (std::holds_alternative<invalid_tag>(value)) {
         node = error_mark_node;
      } else {
         node = get_identifier(std::get<std::string>(value).c_str());
      }
      set_raw_property(name, node);
   }
   
   //
   // Public:
   //
   
   specified<size_t> processed_bitpack_attributes::get_bitcount() const {
      return _get_integral_property<size_t>("bitcount");
   }
   void processed_bitpack_attributes::set_bitcount(specified<size_t> v) {
      _set_integral_property<size_t>("bitcount", v);
   }
   
   specified<intmax_t> processed_bitpack_attributes::get_min() const {
      return _get_integral_property<intmax_t>("min");
   }
   void processed_bitpack_attributes::set_min(specified<intmax_t> v) {
      _set_integral_property<intmax_t>("min", v);
   }
   
   specified<intmax_t> processed_bitpack_attributes::get_max() const {
      return _get_integral_property<intmax_t>("max");
   }
   void processed_bitpack_attributes::set_max(specified<intmax_t> v) {
      _set_integral_property<intmax_t>("max", v);
   }
   
   specified<std::string> processed_bitpack_attributes::get_inherit() const {
      auto node = get_raw_property("inherit");
      if (node == NULL_TREE)
         return {};
      if (node == error_mark_node)
         return invalid_tag{};
      assert(TREE_CODE(node) == STRING_CST);
      return gw::expr::string_constant::from_untyped(node).value();
   }
   void processed_bitpack_attributes::set_inherit(invalid_tag) {
      set_raw_property("inherit", error_mark_node);
   }
   void processed_bitpack_attributes::set_inherit(tree node) {
      if (node == error_mark_node) {
         set_inherit(invalid_tag{});
         return;
      }
      assert(node == NULL_TREE || TREE_CODE(node) == STRING_CST);
      set_raw_property("inherit", node);
   }
   
   specified<std::string> processed_bitpack_attributes::get_transform_pre_pack() const {
      return _get_identifier_property("transform-pre-pack");
   }
   void processed_bitpack_attributes::set_transform_pre_pack(specified<std::string> v) {
      _set_identifier_property("transform-pre-pack", v);
   }
   
   specified<std::string> processed_bitpack_attributes::get_transform_post_unpack() const {
      return _get_identifier_property("transform-post-unpack");
   }
   void processed_bitpack_attributes::set_transform_post_unpack(specified<std::string> v) {
      _set_identifier_property("transform-post-unpack", v);
   }
   
   bool processed_bitpack_attributes::get_omit() const {
      auto node = get_raw_property("omit");
      return node == boolean_true_node;
   }
   void processed_bitpack_attributes::set_omit(bool v) {
      set_raw_property("omit", v ? boolean_true_node : NULL_TREE);
   }
   
   bool processed_bitpack_attributes::get_as_opaque_buffer() const {
      auto node = get_raw_property("as_opaque_buffer");
      return node == boolean_true_node;
   }
   void processed_bitpack_attributes::set_as_opaque_buffer(bool v) {
      set_raw_property("as_opaque_buffer", v ? boolean_true_node : NULL_TREE);
   }
}