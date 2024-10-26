#include "gcc_wrappers/statement_list.h"
#include <cassert>

namespace gcc_wrappers {
   statement_list::statement_list() {
      this->_tree     = alloc_stmt_list();
      this->_iterator = tsi_start(this->_tree);
   }
   
   void statement_list::append_untyped(tree expr) {
      tsi_link_after(&this->_iterator, &expr, TSI_CONTINUE_LINKING);
   }
}