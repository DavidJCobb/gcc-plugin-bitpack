#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/type/container.h"
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
   
   type::container field::member_of() const {
      if (empty())
         return {};
      auto ctxt = DECL_CONTEXT(this->_node);
      if (ctxt != NULL_TREE) {
         //
         // A FIELD_DECL may lack a DECL_CONTEXT if it's still being 
         // built by the compiler internals.
         //
         assert(TREE_CODE(ctxt) == RECORD_TYPE || TREE_CODE(ctxt) == UNION_TYPE);
      }
      
      return type::container::from_untyped(ctxt);
   }
   type::base field::value_type() const {
      if (empty())
         return {};
      return type::base::from_untyped(TREE_TYPE(this->_node));
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
   
   std::string field::pretty_print() const {
      std::string out;
      
      auto container = this->member_of();
      while (!container.empty() && container.name().empty()) {
         //
         // Contained in an anonymous struct?
         //
         auto type_decl = container.declaration();
         container = {};
         if (type_decl.empty())
            break;
         auto context = type_decl.context();
         if (context.empty())
            break;
         switch (context.code()) {
            case RECORD_TYPE:
            case UNION_TYPE:
               container = decltype(container)::from_untyped(context.as_untyped());
               break;
            default:
               break;
         }
      }
      if (!container.empty()) {
         out = container.pretty_print() + "::";
      }
      out += this->name();
      
      return out;
   }
}

#undef _GCC_DECL_FIELD_BIT_OFFSET