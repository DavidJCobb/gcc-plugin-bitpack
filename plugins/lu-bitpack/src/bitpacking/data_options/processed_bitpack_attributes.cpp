#include "bitpacking/data_options/processed_bitpack_attributes.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/builtin_types.h"
#include <cassert>
namespace gw {
   using namespace gcc_wrappers;
}

namespace bitpacking::data_options {
   processed_bitpack_attributes::processed_bitpack_attributes(gw::decl::base decl) {
      assert(!decl.empty());
      this->_node = decl.as_untyped();
   }
   processed_bitpack_attributes::processed_bitpack_attributes(gw::type::base type) {
      assert(!type.empty());
      this->_node = decl.as_untyped();
   }
   
   gw::attribute processed_bitpack_attributes::_get_attribute() {
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
      
      auto map  = tree_cons(NULL_TREE, NULL_TREE, NULL_TREE);
      auto args = tree_cons(NULL_TREE, map,       NULL_TREE);
      
      auto node     = tree_cons(get_identifier("lu bitpack computed"), args, NULL_TREE);
      auto deferred = decl_attributes(&this->_node, node, ATTR_FLAG_INTERNAL, nullptr);
      assert(deferred == NULL_TREE);
      
      return node;
   }
   
   tree processed_bitpack_attributes::_get_map(gw::attribute attr) {
      tree args = TREE_VALUE(attr);
      assert(args != NULL_TREE);
      tree map  = TREE_VALUE(args); // first argument
      assert(map != NULL_TREE);
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
   tree processed_bitpack_attributes::_get_or_create_map_pair(std::string_view key) {
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
      
      pair = tree_cons(get_identifier(key.data()), NULL_TREE, NULL_TREE);
      TREE_CHAIN(back) = pair;
      return pair;
   }
   
   processed_bitpack_attributes::specified<size_t> processed_bitpack_attributes::get_bitcount() const {
      auto item = _get_value_node(_get_attribute());
      if (item == NULL_TREE)
         return {};
      if (item == error_mark_node)
         return invalid_tag{};
      
      assert(TREE_CODE(item) == INTEGER_CST);
      auto opt = gw::expr::integer_constant::from_untyped(item).try_value_unsigned();
      if (!opt.has_value())
         return invalid_tag{};
      return (size_t) *opt;
   }
   void processed_bitpack_attributes::set_bitcount(specified<size_t> v) {
      auto pair  = _get_or_create_map_pair("bitcount");
      if (std::holds_alternative<invalid_tag>(v)) {
         TREE_VALUE(pair) = error_mark_node;
         return;
      }
      if (std::holds_alternative<std::monostate>(v)) {
         TREE_VALUE(pair) = NULL_TREE;
         return;
      }
      
      auto desired = std::get<size_t>(v);
      auto value   = TREE_VALUE(pair);
      if (value != NULL_TREE) {
         assert(TREE_CODE(value) == INTEGER_CST);
         auto opt = gw::expr::integer_constant::from_untyped(item).try_value_unsigned();
         if (opt.has_value() && *opt == desired) {
            return;
         }
      }
      //
      // Overwrite `TREE_VALUE(pair)`. This will leak the old value node, if 
      // it's not used anywhere else. I don't know yet if GCC has anything to 
      // deal with that, or if leaks are by-design. (I do know that removing 
      // attributes from a DECL or TYPE unavoidably leaks them. Nodes can be 
      // reused in all sorts of places, and they don't seem to be refcounted, 
      // so it may not even be possible to avoid leaks within GCC or plug-ins.)
      //
      const auto& ty = gw::builtin_types::get();
      TREE_VALUE(pair) = gw::expr::integer_constant(ty.size, desired);
   }
}