#include "gcc_wrappers/list_node.h"
#include <stdexcept>

namespace gcc_wrappers {
   
   //
   // list_node::iterator
   //
   
   list_node::iterator::iterator(tree n) : _node(n) {}
   
   std::pair<tree, tree> list_node::iterator::operator*() {
      return std::pair(TREE_PURPOSE(this->_node), TREE_VALUE(this->_node));
   }

   list_node::iterator& list_node::iterator::operator++() {
      assert(this->_node != NULL_TREE);
      this->_node = TREE_CHAIN(this->_node);
      return *this;
   }
   list_node::iterator list_node::iterator::operator++(int) const {
      auto it = *this;
      ++it;
      return it;
   }
   
   //
   // list_node
   //
   
   list_node::iterator list_node::begin() {
      return iterator(this->_node);
   }
   list_node::iterator list_node::end() {
      return iterator(NULL_TREE);
   }
   list_node::iterator list_node::at(size_t n) {
      return iterator(this->untyped_nth_kv_pair(n));
   }
   
   size_t list_node::size() const {
      size_t i = 0;
      for(auto item = this->_node; item != NULL_TREE; item = TREE_CHAIN(item))
         ++i;
      return i;
   }
   
   tree list_node::untyped_back() {
      if (empty())
         return NULL_TREE;
      tree item = this->_node;
      tree next = TREE_CHAIN(item);
      while (next != NULL_TREE) {
         item = next;
         next = TREE_CHAIN(next);
      }
      return item;
   }
   tree list_node::untyped_nth_kv_pair(size_t n) {
      if (empty())
         throw std::out_of_range("out-of-bounds indexed access to empty `list_node`");
      
      size_t i = 0;
      for(auto item = this->_node; item != NULL_TREE; item = TREE_CHAIN(item)) {
         if (i == n)
            return item;
      }
      throw std::out_of_range("out-of-bounds indexed access to `list_node`");
   }
   tree list_node::untyped_nth_key(size_t n) {
      return TREE_PURPOSE(untyped_nth_kv_pair(n));
   }
   tree list_node::untyped_nth_value(size_t n) {
      return TREE_VALUE(untyped_nth_kv_pair(n));
   }
}