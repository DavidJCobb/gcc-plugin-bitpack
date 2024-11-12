#pragma once
#include <cassert>
#include <concepts>
#include <string_view>
#include <type_traits>
// GCC:
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_wrappers {
   namespace impl {
      template<typename Wrapper>
      concept has_typecheck = requires (tree t) {
         { Wrapper::node_is(t) } -> std::same_as<bool>;
      };
      
      template<typename Super, typename Sub>
      concept can_is_as = std::is_base_of_v<Super, Sub> && !std::is_same_v<Super, Sub> && has_typecheck<Sub>;
   }
   
   class _wrapped_tree_node {
      protected:
         enum class _from_untyped_tag {};
         
         template<typename Self>
         void _set_from_untyped(tree t) {
            if constexpr (impl::has_typecheck<Self>) {
               if (t != NULL_TREE) {
                  assert(Self::node_is(t));
               }
            }
            this->_node = t;
         }
         
         template<typename Self>
         static Self _from_unwrapped(tree t) {
            static_assert(sizeof(Self) == sizeof(_wrapped_tree_node));
            if constexpr (impl::has_typecheck<Self>) {
               if (t != NULL_TREE) {
                  assert(Self::node_is(t));
               }
            }
            _wrapped_tree_node wrap;
            wrap._node = t;
            return *(Self*)&wrap;
         }
      
      protected:
         tree _node = NULL_TREE;
      
      public:
         inline bool empty() const {
            return this->_node == NULL_TREE;
         }
         
         int code() const {
            return TREE_CODE(this->_node); // TODO: do we need to null check here?
         }
         const std::string_view code_name() const noexcept {
            if (empty())
               return "<null>";
            return get_tree_code_name(TREE_CODE(this->_node));
         }
         
         constexpr const tree& as_untyped() const {
            return this->_node;
         }
         constexpr tree& as_untyped() {
            return this->_node;
         }
         
         inline void set_from_untyped(tree t) {
            this->_node = t;
         }
         
         template<typename Subclass> requires impl::can_is_as<_wrapped_tree_node, Subclass>
         bool is() {
            return !empty() && Subclass::node_is(this->_node);
         }
         
         template<typename Subclass> requires impl::can_is_as<_wrapped_tree_node, Subclass>
         Subclass as() {
            if (is<Subclass>())
               return Subclass::from_untyped(this->_node);
            return Subclass{};
         }
   };
}

// std::hash isn't declared in any one header, so just pick and include a 
// random header that's known to declare it:
#include <optional>

// allow wrapped nodes as keys in containers like `std::unordered_map`
namespace std {
   template<typename T> requires std::is_base_of_v<gcc_wrappers::_wrapped_tree_node, T>
   struct hash<T> {
      size_t operator()(const T& s) const noexcept {
         return hash<void*>()((void*)s.as_untyped());
      }
   };
}