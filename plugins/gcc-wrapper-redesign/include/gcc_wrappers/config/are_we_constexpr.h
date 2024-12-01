#pragma once
#include <version>

namespace gcc_wrappers::config {
   constexpr const bool are_we_constexpr = 
      
      // Can `gcc_wrappers::optional` avoid accessing union members outside 
      // of their lifetimes without needing to waste space on a tag that'd 
      // be unnecessary at run-time?
      #if __cpp_lib_is_within_lifetime >= 202306L
         true
      #else
         false
      #endif
      
      &&
      
      // Has all the node boilerplate, across all subclasses, been made 
      // constexpr? This includes `T::raw_node_is`, `T::is`, `T::as`, and 
      // `T::wrap`.
      false
   ;
}