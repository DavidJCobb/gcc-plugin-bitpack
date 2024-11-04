#include "gcc_wrappers/flow/simple_for_loop.h"
#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/expr/go_to_label.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/label.h"
#include "gcc_wrappers/expr/ternary.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace gcc_wrappers::flow {
   simple_for_loop::simple_for_loop(type::integral counter_type)
      :
      counter("__i", counter_type)
   {
      this->counter.make_artificial();
   }
   
   void simple_for_loop::bake(statement_list&& loop_body) {
      auto counter_type = this->counter.value_type();
      auto statements   = this->enclosing.statements();
      
      /*
      
         Produces:
         
         {
            int __i = start;
            goto l_continue;
         l_body:
            // ...
            __i += increment;
         l_continue:
            (__i <= last) ? goto l_body : goto l_break;
         l_break:
         }
         
      */
      
      this->counter.make_used();
      statements.append(this->counter.make_declare_expr());
      this->counter.set_initial_value(gw::expr::integer_constant(counter_type, this->counter_bounds.start));
      
      gw::decl::label l_body;
      
      statements.append(gw::expr::go_to_label(this->label_continue));
      statements.append(gw::expr::label(l_body));
      statements.append(std::move(loop_body));
      statements.append(
         this->counter.as_value().increment_pre(
            gw::expr::integer_constant(counter_type, this->counter_bounds.increment)
         ).as_expr<gw::expr::base>()
      );
      statements.append(gw::expr::label(this->label_continue));
      statements.append(
         gw::expr::ternary(
            gw::type::base::from_untyped(void_type_node),
            this->counter.as_value().cmp_is_less_or_equal(
               gw::expr::integer_constant(counter_type, this->counter_bounds.last)
            ),
            gw::expr::go_to_label(l_body),
            gw::expr::go_to_label(this->label_break)
         )
      );
      statements.append(gw::expr::label(this->label_break));
   }
}