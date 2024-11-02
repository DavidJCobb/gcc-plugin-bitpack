#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"
#include <cassert>

// _GCC_DECL_FIELD_BIT_OFFSET
#ifdef DECL_FIELD_BITPOS
   #define _GCC_DECL_FIELD_BIT_OFFSET DECL_FIELD_BITPOS
#else
   #define _GCC_DECL_FIELD_BIT_OFFSET DECL_FIELD_BIT_OFFSET
#endif

// _GCC_DECL_IS_BITFIELD
#ifdef DECL_C_BIT_FIELD
   #define _GCC_DECL_IS_BITFIELD DECL_C_BIT_FIELD
#else
   #define _GCC_DECL_IS_BITFIELD DECL_BIT_FIELD
#endif


namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(field)
   
   list_node field::attributes() const {
      return list_node(DECL_ATTRIBUTES(this->_node));
   }
   type field::member_of() const {
      if (empty())
         return {};
      auto ctxt = DECL_CONTEXT(this->_node);
      assert(TREE_CODE(ctxt) == RECORD_TYPE || TREE_CODE(ctxt) == UNION_TYPE);
      
      type t;
      t.set_from_untyped(ctxt);
      return t;
   }
   type field::value_type() const {
      if (empty())
         return {};
      type t;
      t.set_from_untyped(TREE_TYPE(this->_node));
      return t;
   }
   
   size_t field::offset_in_bits() const {
      return TREE_INT_CST_LOW(_GCC_DECL_FIELD_BIT_OFFSET(this->_node));
   }
   size_t field::size_in_bits() const {
      return TREE_INT_CST_LOW(DECL_SIZE(this->_node));
   }
   
   bool field::is_bitfield() const {
      return _GCC_DECL_IS_BITFIELD(this->_node);
   }
}

#undef _GCC_DECL_FIELD_BIT_OFFSET