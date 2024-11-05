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
      return list_length(this->_node);
   }
   
   tree chain_node::untyped_back() {
      return tree_last(this->_node);
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
   
   bool chain_node::contains_untyped(tree t) {
      return chain_member(t, this->_node);
   }
   
   //
   // Functions for mutating a chain:
   //
   
   void chain_node::concat(chain_node other) {
      this->_node = chainon(this->_node, other._node);
   }
   void chain_node::reverse() {
      this->_node = nreverse(this->_node);
   }
}