#include "gcc_wrappers/statement_list.h"
#include "gcc_wrappers/expr/base.h"
#include <cassert>

namespace gcc_wrappers {
   using iterator_type = statement_list::iterator;
   
   //#pragma region iterator
      _wrapped_tree_node iterator_type::operator*() {
         _wrapped_tree_node w;
         w.set_from_untyped(tsi_stmt(this->_data));
         return w;
      }
   
      iterator_type& iterator_type::operator--() {
         tsi_prev(&this->_data);
         return *this;
      }
      iterator_type iterator_type::operator--(int) {
         iterator_type it(*this);
         return --it;
      }
      iterator_type& iterator_type::operator++() {
         assert(!tsi_end_p(this->_data));
         tsi_next(&this->_data);
         return *this;
      }
      iterator_type iterator_type::operator++(int) {
         iterator_type it(*this);
         return ++it;
      }
      
   //#pragma endregion
   
   statement_list::statement_list(tree t) {
      if (t != NULL_TREE) {
         assert(TREE_CODE(t) == STATEMENT_LIST);
      }
      this->_tree     = t;
      this->_iterator = tsi_last(t);
   }
   
   statement_list::statement_list() {
      this->_tree     = alloc_stmt_list();
      this->_iterator = tsi_last(this->_tree);
   }
   
   /*static*/ statement_list statement_list::view_of(tree node) {
      return statement_list(node);
   }
   
   statement_list::iterator statement_list::begin() {
      iterator it;
      it._data = tsi_start(this->_tree);
      return it;
   }
   statement_list::iterator statement_list::end() {
      iterator it;
      it._data     = tsi_start(this->_tree);
      it._data.ptr = nullptr;
      return it;
   }
   
   bool statement_list::empty() const {
      return STATEMENT_LIST_HEAD(this->_tree) == nullptr;
   }
   
   void statement_list::append(const expr::base& expr) {
      assert(!expr.empty());
      tsi_link_after(&this->_iterator, expr.as_untyped(), TSI_CONTINUE_LINKING);
      assert(!empty());
   }
   void statement_list::append(statement_list&& other) {
      tsi_link_after(&this->_iterator, other._tree, TSI_CONTINUE_LINKING);
      other._tree     = NULL_TREE;
      other._iterator = {};
   }
   void statement_list::prepend(const expr::base& expr) {
      assert(!expr.empty());
      auto it = tsi_start(this->_tree);
      tsi_link_before(&it, expr.as_untyped(), TSI_CONTINUE_LINKING);
      this->_tree = it.container;
      assert(!empty());
   }
   void statement_list::prepend(statement_list&& other) {
      auto it = tsi_start(this->_tree);
      tsi_link_after(&it, other._tree, TSI_CONTINUE_LINKING);
      this->_tree = it.container;
      other._tree     = NULL_TREE;
      other._iterator = {};
   }
   
   void statement_list::append_untyped(tree expr) {
      tsi_link_after(&this->_iterator, expr, TSI_CONTINUE_LINKING);
   }
}