#pragma once
#include "gcc_wrappers/node.h"
#include "gcc_wrappers/node_pointer_template.h"

namespace gcc_wrappers {
   //
   // Think of this class as a view.
   //
   class chain : public node {
      protected:
         node_ptr _node;
      
      public:
         class iterator {
            friend chain;
            protected:
               iterator(node_ptr);
               
               node_ptr _node;
               
            public:
               node operator*();
            
               iterator& operator++();
               iterator  operator++(int) const;
               iterator& operator+=(size_t);
               iterator  operator+(size_t) const;
               
               bool operator==(const iterator&) const = default;
         };
         
      public:
         chain() {}
         chain(node);
         chain(node_ptr);
      
         iterator begin();
         iterator end();
         iterator at(size_t n);
         
         size_t size() const;
         
         // Accessors. May throw std::out_of_range.
         node front(); // The `front` is just the node that we're viewing.
         node back();
         node operator[](size_t n);
         
         // The functor receives an argument of type `node`. If the functor 
         // returns bool, then falsy values cause us to stop iterating early. 
         // Other return types are permitted and ignored.
         //
         // We also support passing the current index if the functor takes 
         // an additional parameter of type `size_t`.
         template<typename Functor>
         void for_each(Functor&&);
         
         bool contains(node) const;
         
         //
         // Functions for mutating a chain:
         //
         
         void concat(chain);
         void reverse();
   };
}

#include "gcc_wrappers/chain.inl"