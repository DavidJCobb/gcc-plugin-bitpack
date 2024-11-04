#include "gcc_wrappers/chain_node.h"
#include <stdexcept>

namespace gcc_wrappers {
   
   //
   // chain_node::iterator
   //
   
   chain_node::iterator::iterator(tree n) : _node(n) {}
   
   tree chain_node::iterator::operator*() {
      return this->_node;
   }

   chain_node::iterator& chain_node::iterator::operator++() {
      assert(this->_node != NULL_TREE);
      this->_node = TREE_CHAIN(this->_node);
      return *this;
   }
   chain_node::iterator chain_node::iterator::operator++(int) const {
      auto it = *this;
      ++it;
      return it;
   }
   
   //
   // chain_node
   //
   
   chain_node::iterator chain_node::begin() {
      return iterator(this->_node);
   }
   chain_node::iterator chain_node::end() {
      return iterator(NULL_TREE);
   }
   chain_node::iterator chain_node::at(size_t n) {
      return iterator(this->untyped_nth(n));
   }
   
   size_t chain_node::size() const {
      size_t i = 0;
      for(auto item = this->_node; item != NULL_TREE; item = TREE_CHAIN(item))
         ++i;
      return i;
   }
   
   tree chain_node::untyped_back() {
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
   tree chain_node::untyped_nth(size_t n) {
      if (empty())
         throw std::out_of_range("out-of-bounds indexed access to empty `chain_node`");
      
      size_t i = 0;
      for(auto item = this->_node; item != NULL_TREE; item = TREE_CHAIN(item)) {
         if (i == n)
            return item;
      }
      throw std::out_of_range("out-of-bounds indexed access to `chain_node`");
   }
}