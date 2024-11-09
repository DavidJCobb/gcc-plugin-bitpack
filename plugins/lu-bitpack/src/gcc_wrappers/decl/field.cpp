#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/scope.h"
#include "gcc_wrappers/_boilerplate-impl.define.h"
#include <cassert>

//
// GCC has renamed some macros; prefer the new names over the old. 
// `#define` synonyms so we can avoid scattering preprocessor if/else 
// checks in the code below; then `#undef` them at the bottom of the 
// file.
//

// _GCC_DECL_FIELD_BIT_OFFSET
#ifdef DECL_FIELD_BITPOS
   #define _GCC_DECL_FIELD_BIT_OFFSET DECL_FIELD_BITPOS
#else
   //
   // Oldest known/supported name.
   //
   #ifndef DECL_FIELD_BIT_OFFSET
      #error GCC may have renamed the macro used to get a FIELD_DECL&apos;s offset in bits. Update the code here to cope with that.
   #endif
   #define _GCC_DECL_FIELD_BIT_OFFSET DECL_FIELD_BIT_OFFSET
#endif

// _GCC_DECL_IS_BITFIELD
#ifdef DECL_C_BIT_FIELD
   #define _GCC_DECL_IS_BITFIELD DECL_C_BIT_FIELD
#else
   //
   // Oldest known/supported name.
   //
   #ifndef DECL_BIT_FIELD
      #error GCC may have renamed the macro used to get a C bitfield FIELD_DECL&apos;s bit width. Update the code here to cope with that.
   #endif
   #define _GCC_DECL_IS_BITFIELD DECL_BIT_FIELD
#endif


namespace gcc_wrappers::decl {
   WRAPPED_TREE_NODE_BOILERPLATE(field)
   
   type::container field::member_of() const {
      if (empty())
         return {};
      auto scope = this->context();
      if (scope.empty())
         //
         // A FIELD_DECL may lack a DECL_CONTEXT if it's still being 
         // built by the compiler internals.
         //
         return {};
      if (scope.is_type()) {
         auto type = scope.as_type();
         if (type.is_record() || type.is_union())
            return type::container::from_untyped(type.as_untyped());
      }
      return {};
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
   
   bool field::is_class_vtbl_pointer() const {
      return !empty() && DECL_VIRTUAL_P(this->_node);
   }
}

#undef _GCC_DECL_FIELD_BIT_OFFSET
#undef _GCC_DECL_IS_BITFIELD