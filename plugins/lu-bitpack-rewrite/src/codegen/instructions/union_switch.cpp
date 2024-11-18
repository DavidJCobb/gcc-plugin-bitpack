#include "codegen/instructions/union_switch.h"
#include <cassert>
#include "codegen/instruction_generation_context.h"
#include "gcc_wrappers/expr/local_block.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/ternary.h"
#include "gcc_wrappers/builtin_types.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace codegen::instructions {
   /*virtual*/ expr_pair union_switch::generate(const instruction_generation_context& ctxt) const {
      const auto& ty = gw::builtin_types::get();
      
      auto operand      = this->condition_operand.as_value_pair();
      auto operand_type = operand.read.value_type();
      assert(operand_type.is_boolean() || operand_type.is_enum() || operand_type.is_integer());
      
      auto root_read = gw::expr::ternary::from_untyped(NULL_TREE);
      auto root_save = gw::expr::ternary::from_untyped(NULL_TREE);
      auto prev_read = gw::expr::ternary::from_untyped(NULL_TREE);
      auto prev_save = gw::expr::ternary::from_untyped(NULL_TREE);
      for(auto& pair : this->cases) {
         auto pair_rhs  = gw::expr::integer_constant(operand_type.as_integral(), pair.first);
         auto pair_gene = pair.second->generate(ctxt);
         
         auto tern_read = gw::expr::ternary(
            ty.basic_void,
            
            // condition:
            operand.read.cmp_is_equal(pair_rhs),
            
            // branches:
            pair_gene.read,
            {}
         );
         auto tern_save = gw::expr::ternary(
            ty.basic_void,
            
            // condition:
            operand.save.cmp_is_equal(pair_rhs),
            
            // branches:
            pair_gene.save,
            {}
         );
         
         if (prev_read.empty()) {
            root_read = tern_read;
            root_save = tern_save;
         } else {
            assert(!prev_save.empty());
            prev_read.set_false_branch(tern_read);
            prev_save.set_false_branch(tern_save);
         }
         prev_read = tern_read;
         prev_save = tern_save;
      }
      return expr_pair{
         .read = root_read,
         .save = root_save,
      };
   }
}