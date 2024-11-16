#pragma once
#include <cstdint>
#include <unordered_map>
#include "codegen/instructions/base.h"
#include "codegen/instructions/union_case.h"

namespace codegen::instructions {
   class union_switch : public base {
      public:
         static constexpr const type node_type = type::union_switch;
         virtual type get_type() const noexcept override { return node_type; };
      
      public:
         value_path condition_operand;
         std::unordered_map<intmax_t, std::unique_ptr<union_case>> cases;
   };
}