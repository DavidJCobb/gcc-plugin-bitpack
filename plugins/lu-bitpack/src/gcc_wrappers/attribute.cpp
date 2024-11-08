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
   
   _wrapped_tree_node attribute::arguments_wrapper::iterator::operator*() {
      assert(this->_node != NULL_TREE);
      auto arg_value_node = TREE_VALUE(this->_node);
      
      _wrapped_tree_node out;
      out.set_from_untyped(arg_value_node);
      return out;
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

   size_t attribute::arguments_wrapper::size() const noexcept {
      auto   node = this->_node;
      size_t i    = 0;
      while (node != NULL_TREE) {
         ++i;
         node = TREE_CHAIN(node);
      }
      return i;
   }
   _wrapped_tree_node attribute::arguments_wrapper::operator[](size_t n) {
      auto node = this->_node;
      if (node == NULL_TREE)
         return {};
      
      for(size_t i = 0; i < n; ++i) {
         node = TREE_CHAIN(node);
         if (node == NULL_TREE)
            return {};
      }
      _wrapped_tree_node out;
      out.set_from_untyped(TREE_VALUE(node));
      return out;
   }
   
   _wrapped_tree_node attribute::arguments_wrapper::front() {
      assert(!empty());
      return *begin();
   }
   _wrapped_tree_node attribute::arguments_wrapper::back() {
      return std::as_const(*this).back();
   }
   const _wrapped_tree_node attribute::arguments_wrapper::front() const {
      return const_cast<arguments_wrapper&>(*this).front();
   }
   const _wrapped_tree_node attribute::arguments_wrapper::back() const {
      assert(!empty());
      auto node = this->_node;
      if (node != NULL_TREE) {
         while (TREE_CHAIN(node) != NULL_TREE) {
            node = TREE_CHAIN(node);
         }
      }
      _wrapped_tree_node out;
      out.set_from_untyped(TREE_VALUE(node));
      return out;
   }

   attribute::arguments_wrapper::iterator attribute::arguments_wrapper::begin() {
      return iterator(this->_node);
   }
   attribute::arguments_wrapper::iterator attribute::arguments_wrapper::end() {
      return iterator(NULL_TREE);
   }
   attribute::arguments_wrapper::iterator attribute::arguments_wrapper::at(size_t n) {
      auto node = this->_node;
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
      return std::as_const(*this).arguments();
   }
   const attribute::arguments_wrapper attribute::arguments() const {
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
      return std::as_const(*this).first_attribute_with_prefix(prefix);
   }
   const attribute attribute_list::first_attribute_with_prefix(lu::strings::zview prefix) const {
      return attribute::from_untyped(lookup_attribute_by_prefix(prefix.c_str(), this->_node));
   }
   
   attribute attribute_list::get_attribute(lu::strings::zview name) {
      return std::as_const(*this).get_attribute(name);
   }
   const attribute attribute_list::get_attribute(lu::strings::zview name) const {
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