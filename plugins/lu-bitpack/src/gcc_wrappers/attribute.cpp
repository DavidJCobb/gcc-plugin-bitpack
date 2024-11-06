#include "gcc_wrappers/attribute.h"
#include <stdexcept>
#include <gcc-plugin.h>
#include <tree.h>
#include <stringpool.h> // dependency for <attribs.h>
#include <attribs.h>

namespace gcc_wrappers {
   
   //
   // attribute::arguments_wrapper::iterator
   //
   
   attribute::arguments_wrapper::iterator::iterator(tree t) : _node(t) {}
   
   expr::base attribute::arguments_wrapper::iterator::operator*() {
      assert(this->_node != NULL_TREE);
      auto arg_value_node = TREE_VALUE(this->_node);
      return expr::base::from_untyped(arg_value_node);
   }

   attribute::arguments_wrapper::iterator& attribute::arguments_wrapper::iterator::operator++() {
      assert(this->_node != NULL_TREE);
      this->_node = TREE_CHAIN(this->_node);
      return *this;
   }
   attribute::arguments_wrapper::iterator  attribute::arguments_wrapper::iterator::operator++(int) const {
      auto it = *this;
      ++it;
      return it;
   }
   
   //
   // attribute::arguments_wrapper
   //
   
   attribute::arguments_wrapper::arguments_wrapper(tree t) : _node(t) {
   }
   
   bool attribute::arguments_wrapper::empty() const noexcept {
      return this->_node == NULL_TREE;
   }

   bool attribute::arguments_wrapper::has_leading_identifier() const {
      if (empty())
         return false;
      auto first = TREE_VALUE(this->_node);
      if (first == NULL_TREE)
         return false;
      return TREE_CODE(first) == IDENTIFIER_NODE;
   }
   const_tree attribute::arguments_wrapper::leading_identifier_node() const {
      return has_leading_identifier() ? TREE_VALUE(this->_node) : NULL_TREE;
   }
   std::string_view attribute::arguments_wrapper::leading_identifier() const {
      auto id = leading_identifier_node();
      if (id == NULL_TREE)
         return {};
      return std::string_view(IDENTIFIER_POINTER(id));
   }
   
   size_t attribute::arguments_wrapper::size() const noexcept {
      auto node = this->_node;
      if (has_leading_identifier())
         node = TREE_CHAIN(node);
      
      size_t i = 0;
      while (node != NULL_TREE) {
         ++i;
         node = TREE_CHAIN(node);
      }
      return i;
   }
   expr::base attribute::arguments_wrapper::operator[](size_t n) {
      auto node = this->_node;
      if (has_leading_identifier())
         node = TREE_CHAIN(node);
      if (node == NULL_TREE)
         return {};
      
      for(size_t i = 0; i < n; ++i) {
         node = TREE_CHAIN(node);
         if (node == NULL_TREE)
            return {};
      }
      return expr::base::from_untyped(TREE_VALUE(node));
   }

   expr::base attribute::arguments_wrapper::front() {
      assert(!empty());
      return *begin();
   }
   expr::base attribute::arguments_wrapper::back() {
      assert(!empty());
      auto node = this->_node;
      if (has_leading_identifier())
         node = TREE_CHAIN(node);
      if (node != NULL_TREE) {
         while (TREE_CHAIN(node) != NULL_TREE) {
            node = TREE_CHAIN(node);
         }
      }
      return expr::base::from_untyped(TREE_VALUE(node));
   }

   attribute::arguments_wrapper::iterator attribute::arguments_wrapper::begin() {
      auto node = this->_node;
      if (has_leading_identifier())
         node = TREE_CHAIN(node);
      return iterator(node);
   }
   attribute::arguments_wrapper::iterator attribute::arguments_wrapper::end() {
      return iterator(NULL_TREE);
   }
   attribute::arguments_wrapper::iterator attribute::arguments_wrapper::at(size_t n) {
      auto node = this->_node;
      if (has_leading_identifier())
         node = TREE_CHAIN(node);
      assert(node != NULL_TREE);
      if (node == NULL_TREE)
         throw std::out_of_range("out-of-bounds access to an attribute's argument expressions");
      
      for(size_t i = 0; i < n; ++i) {
         node = TREE_CHAIN(node);
         if (node == NULL_TREE)
            throw std::out_of_range("out-of-bounds access to an attribute's argument expressions");
      }
      return iterator(node);
   }

   //
   // attribute
   //
   
   /*static*/ attribute from_untyped(tree t) {
      attribute out;
      out.set_from_untyped(t);
      return out;
   }
   
   std::string_view attribute::name() const {
      if (empty())
         return {};
      
      auto id_node = get_attribute_name(this->_node);
      if (TREE_CODE(id_node) != IDENTIFIER_NODE)
         return {};
      return IDENTIFIER_POINTER(id_node);
   }
   std::string_view attribute::namespace_name() const {
      if (empty())
         return {};
      
      auto id_node = get_attribute_namespace(this->_node);
      if (TREE_CODE(id_node) != IDENTIFIER_NODE)
         return {};
      return IDENTIFIER_POINTER(id_node);
   }
   
   bool attribute::is_cpp11() const {
      return cxx11_attribute_p(this->_node);
   }
   
   bool attribute::compare_values(const attribute other) const {
      if (this->empty())
         return other.empty();
      if (other.empty())
         return false;
      return attribute_value_equal(this->as_untyped(), other.as_untyped());
   }
         
   attribute::arguments_wrapper attribute::arguments() {
      return arguments_wrapper(empty() ? NULL_TREE : TREE_VALUE(this->_node));
   }
   
   //
   // attribute_list::iterator
   //
   
   attribute_list::iterator::iterator(tree n) : _node(n) {}
   
   attribute attribute_list::iterator::operator*() {
      return attribute::from_untyped(this->_node);
   }

   attribute_list::iterator& attribute_list::iterator::operator++() {
      assert(this->_node != NULL_TREE);
      this->_node = TREE_CHAIN(this->_node);
      return *this;
   }
   attribute_list::iterator attribute_list::iterator::operator++(int) const {
      auto it = *this;
      ++it;
      return it;
   }
   
   //
   // attribute_list
   //
   
   /*static*/ attribute_list attribute_list::from_untyped(tree t) {
      attribute_list out;
      out._node = t;
      return out;
   }
   
   bool attribute_list::contains(const attribute attr) const {
      if (attr.empty() || this->empty())
         return false;
      return attribute_list_contained(this->_node, attr.as_untyped());
   }
   
   attribute_list::iterator attribute_list::begin() {
      return iterator(this->_node);
   }
   attribute_list::iterator attribute_list::end() {
      return iterator(NULL_TREE);
   }
   attribute_list::iterator attribute_list::at(size_t n) {
      auto node = this->_node;
      if (node == NULL_TREE)
         throw std::out_of_range("out-of-bounds access to an attribute's argument expressions");
      
      for(size_t i = 0; i < n; ++i) {
         node = TREE_CHAIN(node);
         if (node == NULL_TREE)
            throw std::out_of_range("out-of-bounds access to an attribute's argument expressions");
      }
      return iterator(node);
   }
   
   attribute attribute_list::first_attribute_with_prefix(lu::strings::zview prefix) {
      return attribute::from_untyped(lookup_attribute_by_prefix(prefix.c_str(), this->_node));
   }
   
   attribute attribute_list::get_attribute(lu::strings::zview name) {
      return attribute::from_untyped(lookup_attribute(name.c_str(), this->_node));
   }
   
   bool attribute_list::has_attribute(lu::strings::zview name) const {
      auto attr = const_cast<attribute_list&>(*this).get_attribute(name);
      return !attr.empty();
   }
         
   void attribute_list::remove_attribute(lu::strings::zview name) {
      if (empty())
         return;
      this->_node = ::remove_attribute(name.c_str(), this->_node);
   }
   
   bool attribute_list::operator==(const attribute_list other) const noexcept {
      if (this->empty())
         return other.empty();
      else if (other.empty())
         return false;
      return attribute_list_equal(this->_node, other._node);
   }
}