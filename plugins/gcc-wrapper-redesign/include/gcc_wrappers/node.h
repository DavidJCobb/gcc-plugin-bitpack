#pragma once
#include <concepts>
#include <string_view>
#include <type_traits>
#include <version>
// GCC:
#include <gcc-plugin.h>
#include <tree.h>

namespace gcc_wrappers {
   namespace impl {
      template<typename Wrapper>
      concept has_typecheck = requires (tree t) {
         { Wrapper::raw_node_is(t) } -> std::same_as<bool>;
      };
      
      template<typename Super, typename Sub>
      concept can_is_as = std::is_base_of_v<Super, Sub> && !std::is_same_v<Super, Sub> && has_typecheck<Sub>;
   }
   
   class node {
      protected:
         tree _node = NULL_TREE;
         
         node() {}
         
      public:
         static node wrap(tree n);
         
         node(const node& o) = default;
         node(node&&) noexcept = delete;
         node& operator=(const node&) = default;
         node& operator=(node&&) noexcept = delete;
         
         // Identity comparison.
         constexpr bool is_same(const node& o) const noexcept {
            return this->as_raw() == o.as_raw();
         }
         
         // Identity comparison by default. Some subclasses may override this 
         // to do an equality comparison instead, which is why `is_same` has 
         // been provided separately.
         constexpr bool operator==(const node& o) const noexcept {
            return this->is_same(o);
         }
      
         tree_code code() const;
         const std::string_view code_name() const noexcept;
         
         const_tree as_raw() const;
         tree as_raw();
         
         template<typename Subclass> requires impl::can_is_as<node, Subclass>
         bool is() {
            return Subclass::node_is(this->_node);
         }
         
         template<typename Subclass> requires impl::can_is_as<node, Subclass>
         Subclass as() {
            if (is<Subclass>())
               return Subclass::wrap(this->_node);
            return Subclass{};
         }
   };
   
   template<typename T>
   concept is_node_reference_wrapper = requires() {
      requires std::is_base_of_v<node, T>;
      requires sizeof(T) == sizeof(node);
   };
}