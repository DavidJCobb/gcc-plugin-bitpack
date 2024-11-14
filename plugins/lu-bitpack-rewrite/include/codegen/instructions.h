#pragma once
#include <memory>
#include <vector>
#include "codegen/decl_described_item.h"
#include "codegen/expr_pair.h"

namespace codegen::instructions {
   class base {
      public:
         virtual ~base() {}
   };
}
namespace codegen {
   using instruction_list = std::vector<std::unique_ptr<instructions::base>>;
}
   
namespace codegen::instructions {
   // Function root
   class root : public base {
      public:
         instruction_list instructions;
   };
   
   class single : public base {
      public:
         decl_described_item object;
   };
   
   class indexed_group : public base {
      public:
         struct {
            decl_described_item object;
            size_t start = 0;
            size_t count = 0;
         } array;
         
         // Executed per element.
         instruction_list instructions;
   };
   
   class transform : public base {
      public:
         std::vector<gcc_wrappers::type::base> transformations;
         struct {
            decl_described_item to_be_transformed;
            decl_described_item final_transformed;
         } objects;
         
         // Executed on the final transformed object.
         instruction_list instructions;
   };
   
   class branch : public base {
      public:
         struct {
            decl_described_item operand;
            intmax_t            value = 0;
         } condition;
         
         // Executed if the condition is met.
         instruction_list instructions;
         
         // Executed if the condition is not met.
         branch* alternative = nullptr;
   };
}