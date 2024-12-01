#pragma once
#include <cassert>
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
      
      template<typename Wrapper>
      void do_typecheck(tree t) {
         if constexpr (has_typecheck<Wrapper>) {
            assert(Wrapper::raw_node_is(t));
         }
      }
   }
   
   class node {
      protected:
         tree _node = NULL_TREE;
         
         node() {}
         
      public:
         static node wrap(tree n);
         
         // Identity comparison.
         constexpr bool is_same(const node&) const noexcept;
         
         // Identity comparison by default. Some subclasses may override this 
         // to do an equality comparison instead, which is why `is_same` has 
         // been provided separately.
         constexpr bool operator==(const node&) const noexcept;
      
         tree_code code() const;
         const std::string_view code_name() const noexcept;
         
         constexpr const_tree unwrap() const;
         constexpr tree unwrap();
         
         template<typename Subclass> requires impl::can_is_as<node, Subclass>
         bool is() {
            return Subclass::raw_node_is(this->_node);
         }
         
         template<typename Subclass> requires impl::can_is_as<node, Subclass>
         Subclass as() {
            assert(is<Subclass>());
            return Subclass::wrap(this->_node);
         }
   };
}

#include "gcc_wrappers/node.inl"