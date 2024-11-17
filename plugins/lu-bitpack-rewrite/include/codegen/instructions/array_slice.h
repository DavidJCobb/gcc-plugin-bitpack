#pragma once
#include "codegen/instructions/base.h"
#include "codegen/decl_pair.h"
#include "codegen/value_path.h"
#include "gcc_wrappers/decl/variable.h"

namespace codegen::instructions {
   //
   // Represents access into an array via a for-loop. This doesn't represent 
   // actually serializing an array element (because we may instead serialize 
   // members of that element); it only represents the for-loop itself.
   //
   // NOTE: `array.count` may be 1. In that case, we ideally wouldn't generate 
   // a for-loop; we'd "unroll" ourselves. However, child and descendant nodes 
   // may refer to our loop-index VAR_DECLs; we'd need some sort of state that 
   // recursive codegen calls could use to say, "Oh, that VAR_DECL should be 
   // the integer constant X. I will just access X instead."
   //
   class array_slice : public container {
      public:
         array_slice();
         
         static constexpr const type node_type = type::array_slice;
         virtual type get_type() const noexcept override { return node_type; };
         
         virtual expr_pair generate(const instruction_generation_context&) const;
         
      public:
         struct {
            value_path value;
            size_t     start = 0;
            size_t     count = 0;
         } array;
         struct {
            struct {
               gcc_wrappers::decl::variable read; // VAR_DECL
               gcc_wrappers::decl::variable save; // VAR_DECL
            } variables;
            decl_pair descriptors;
         } loop_index;
   };
}