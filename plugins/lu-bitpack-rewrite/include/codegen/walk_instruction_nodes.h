#pragma once
#include "codegen/instructions/base.h"
#include "codegen/instructions/union_switch.h"

namespace codegen {
   template<typename Functor>
   void walk_instruction_nodes(Functor&& functor, instructions::base& node) {
      functor(node);
      if (auto* cont = node.as<instructions::container>()) {
         for(auto& child_ptr : cont->instructions)
            walk_instruction_nodes(functor, *child_ptr.get());
      } else if (auto* us = node.as<instructions::union_switch>()) {
         for(const auto& pair : us->cases)
            walk_instruction_nodes(functor, *pair.second.get());
      }
   }
   
   template<typename Functor>
   void walk_instruction_nodes(Functor&& functor, const instructions::base& node) {
      functor(node);
      if (auto* cont = node.as<instructions::container>()) {
         for(auto& child_ptr : cont->instructions)
            walk_instruction_nodes(functor, *child_ptr.get());
      } else if (auto* us = node.as<instructions::union_switch>()) {
         for(const auto& pair : us->cases)
            walk_instruction_nodes(functor, *pair.second.get());
      }
   }
}