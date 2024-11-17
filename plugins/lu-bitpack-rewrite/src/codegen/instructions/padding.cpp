#include "codegen/instructions/padding.h"
#include "codegen/instruction_generation_context.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/local_block.h"
#include "gcc_wrappers/builtin_types.h"
#include "bitpacking/global_options.h"
#include "basic_global_state.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace codegen::instructions {
   /*virtual*/ expr_pair padding::generate(const instruction_generation_context& ctxt) const {
      const auto& ty     = gw::builtin_types::get();
      const auto& global = basic_global_state::get().global_options.computed;
      
      size_t remaining = this->bitcount;
      if (remaining == 0)
         return {};
      
      auto _make_next_call = [&ctxt, &ty, &global, &remaining]() {
         gw::decl::function read_func;
         gw::decl::function save_func;
         
         size_t consumed = (std::min)(remaining, (size_t)32);
         if (remaining <= 8) {
            read_func = global.functions.read.u8;
            save_func = global.functions.save.u8;
         } else if (remaining <= 16) {
            read_func = global.functions.read.u16;
            save_func = global.functions.save.u16;
         } else {
            read_func = global.functions.read.u32;
            save_func = global.functions.save.u32;
         }
         
         remaining -= consumed;
         return expr_pair{
            .read = gw::expr::call(
               read_func,
               // args:
               ctxt.state_ptr.read,
               gw::expr::integer_constant(ty.uint8, consumed)
            ),
            .save = gw::expr::call(
               save_func,
               // args:
               ctxt.state_ptr.save,
               gw::expr::integer_constant(ty.basic_int, 0),
               gw::expr::integer_constant(ty.uint8, consumed)
            ),
         };
      };
      
      if (remaining <= 32) {
         return _make_next_call();
      }
      
      gw::expr::local_block block_read;
      gw::expr::local_block block_save;
      auto statements_read = block_read.statements();
      auto statements_save = block_save.statements();
      while (remaining > 0) {
         auto pair = _make_next_call();
         statements_read.append(pair.read);
         statements_save.append(pair.save);
      }
      return expr_pair{
         .read = block_read,
         .save = block_save,
      };
   }
}