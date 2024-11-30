#include "gcc_wrappers/list.h"
#include <stdexcept>

namespace gcc_wrappers {
   
   //
   // list::iterator
   //
   
   list::iterator::iterator(node_ptr n) : _node(n) {}
   
   std::pair<node_ptr, node_ptr> list::iterator::operator*() {
      return std::pair(TREE_PURPOSE(this->_node.unwrap()), TREE_VALUE(this->_node.unwrap()));
   }

   list::iterator& list::iterator::operator++() {
      assert(this->_node != nullptr);
      this->_node = TREE_CHAIN(this->_node.unwrap());
      return *this;
   }
   list::iterator list::iterator::operator++(int) const {
      auto it = *this;
      ++it;
      return it;
   }
   
   //
   // list
   //
   
   list::iterator list::begin() {
      return iterator(this->_node);
   }
   list::iterator list::end() {
      return iterator(nullptr);
   }
   list::iterator list::at(size_t n) {
      return iterator(this->nth_pair(n));
   }
   
   size_t list::size() const {
      return list_length(this->_node.unwrap());
   }
   
   node list::front() {
      if (empty())
         throw std::out_of_range("out-of-bounds indexed access to empty `list`");
      return *this->_node;
   }
   node list::back() {
      if (empty())
         throw std::out_of_range("out-of-bounds indexed access to empty `list`");
      return node::wrap(tree_last(this->_node.unwrap()));
   }
   node list::nth_kv_pair(size_t n) {
      size_t i = 0;
      for(auto raw = this->_node.unwrap(); raw != NULL_TREE; raw = TREE_CHAIN(raw)) {
         if (i == n)
            return node::wrap(raw);
      }
      throw std::out_of_range("out-of-bounds indexed access to `list`");
   }
   node_ptr list::nth_key(size_t n) {
      return TREE_PURPOSE(untyped_nth_kv_pair(n));
   }
   node_ptr list::nth_value(size_t n) {
      return TREE_VALUE(untyped_nth_kv_pair(n));
   }
   
   node_ptr list::find_value_by_key(node_ptr key) {
      auto raw = purpose_member(key.unwrap(), this->_node);
      if (raw != NULL_TREE)
         raw = TREE_VALUE(raw);
      return raw;
   }
   node_ptr list::find_pair_by_key(node_ptr key) {
      return purpose_member(key.unwrap(), this->_node);
   }
   node_ptr list::find_pair_by_value(node_ptr value) {
      return value_member(value.unwrap(), this->_node);
   }
   
   //
   // Functions for mutating a list:
   //
   
   void list::append(node_ptr key, node_ptr value) {
      auto node = this->_node.unwrap();
      auto prev = NULL_TREE;
      while (node != NULL_TREE) {
         prev = node;
         node = TREE_CHAIN(node);
      }
      auto item = tree_cons(key.unwrap(), value.unwrap(), NULL_TREE);
      if (prev == NULL_TREE) {
         this->_node = item;
      } else {
         TREE_CHAIN(prev) = item;
      }
   }
   
   void list::concat(list other) {
      this->_node = chainon(this->_node.unwrap(), other._node.unwrap());
   }
   
   void list::prepend(node_ptr key, node_ptr value) {
      this->_node = tree_cons(key.unwrap(), value.unwrap(), this->_node.unwrap());
   }
}