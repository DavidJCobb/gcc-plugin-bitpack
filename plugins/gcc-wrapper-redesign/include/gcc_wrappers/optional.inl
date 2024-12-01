#pragma once
#include "gcc_wrappers/optional.h"

#define CLASS_TEMPLATE_PARAMS template<typename Wrapper>
#define CLASS_NAME optional<Wrapper>

namespace gcc_wrappers {
   CLASS_TEMPLATE_PARAMS
   void CLASS_NAME::_set_tree(tree raw) {
      if (std::is_constant_evaluated()) {
         if (raw == NULL_TREE) {
            this->_tree = NULL_TREE;
         } else {
            this->_node = value_type::wrap(raw);
         }
         return;
      }
      if constexpr (impl::has_typecheck<value_type>) {
         assert(value_type::raw_node_is(raw));
      }
      this->_tree = raw;
   }
   
   // Copy-construct from optional.
   CLASS_TEMPLATE_PARAMS
   template<typename Subclass>
   constexpr CLASS_NAME::optional(const optional<Subclass>& o)
      requires std::is_base_of_v<typename CLASS_NAME::value_type, Subclass>
   {
      this->_set_tree((tree)o.unwrap()); // unwrapped may be const
   }
   
   // Move-construct from optional.
   CLASS_TEMPLATE_PARAMS
   template<typename Subclass>
   constexpr CLASS_NAME::optional(optional<Subclass>&& o) noexcept
      requires std::is_base_of_v<typename CLASS_NAME::value_type, Subclass>
   {
      this->_set_tree((tree)o.unwrap()); // unwrapped may be const
      o._tree = NULL_TREE;
   }
   
   // Construct from wrapper.
   CLASS_TEMPLATE_PARAMS
   template<typename Subclass>
   constexpr CLASS_NAME::optional(const Subclass& o)
      requires std::is_base_of_v<typename CLASS_NAME::value_type, Subclass>
   {
      this->_set_tree((tree)o.unwrap()); // unwrapped may be const
   }
   
   // Construct from bare node pointer.
   CLASS_TEMPLATE_PARAMS
   CLASS_NAME::optional(tree t) {
      this->_set_tree(t);
   }
   
   CLASS_TEMPLATE_PARAMS
   constexpr bool CLASS_NAME::empty() const noexcept {
      #if __cpp_lib_is_within_lifetime >= 202306L
         if (std::is_constant_evaluated()) {
            return std::is_within_lifetime(&this->_tree);
         }
      #endif
      return this->_tree == NULL_TREE;
   }
   
   CLASS_TEMPLATE_PARAMS
   constexpr tree CLASS_NAME::unwrap() {
      if (std::is_constant_evaluated()) {
         if (empty())
            return NULL_TREE;
         return this->_node.unwrap();
      } else {
         return this->_tree;
      }
   }
   
   CLASS_TEMPLATE_PARAMS
   constexpr const_tree CLASS_NAME::unwrap() const {
      if (std::is_constant_evaluated()) {
         if (empty())
            return NULL_TREE;
         return this->_node.unwrap();
      } else {
         return this->_tree;
      }
   }
   
   CLASS_TEMPLATE_PARAMS
   template<typename Subclass>
   constexpr optional<Subclass> CLASS_NAME::as()
      requires std::is_base_of_v<typename CLASS_NAME::value_type, Subclass>
   {
      if (empty())
         return {};
      if constexpr (impl::has_typecheck<Subclass>) {
         if (!Subclass::raw_node_is(this->_node)) {
            return {};
         }
      }
      return optional<Subclass>(this->_node);
   }
   
   CLASS_TEMPLATE_PARAMS
   CLASS_NAME::value_type CLASS_NAME::operator*() {
      assert(!empty());
      return this->_node;
   }
   
   CLASS_TEMPLATE_PARAMS
   const CLASS_NAME::value_type CLASS_NAME::operator*() const {
      assert(!empty());
      return this->_node;
   }
   
   CLASS_TEMPLATE_PARAMS
   CLASS_NAME::value_type* CLASS_NAME::operator->() {
      if (std::is_constant_evaluated()) {
         if (empty()) {
            throw "invalid compile-time dereferencing of empty optional";
         }
      } else {
         assert(!empty());
      }
      return &this->_node;
   }
   
   CLASS_TEMPLATE_PARAMS
   const CLASS_NAME::value_type* CLASS_NAME::operator->() const {
      if (std::is_constant_evaluated()) {
         if (empty()) {
            throw "invalid compile-time dereferencing of empty optional";
         }
      } else {
         assert(!empty());
      }
      return &this->_node;
   }
   
   CLASS_TEMPLATE_PARAMS
   constexpr CLASS_NAME::operator bool() const noexcept {
      return !empty();
   }
}

#undef CLASS_NAME
#undef CLASS_TEMPLATE_PARAMS