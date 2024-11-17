#include "codegen/instructions/transform.h"
#include "lu/strings/printf_string.h"
#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/expr/local_block.h"
#include "gcc_wrappers/builtin_types.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace codegen::instructions {
   transform::transform(const std::vector<gw::type::base>& types) {
      this->types = types;
      assert(!types.empty());
      
      this->transformed.variables.read = gw::decl::variable(
         "__transformed_read",
         types.back()
      );
      this->transformed.variables.save = gw::decl::variable(
         "__transformed_save",
         types.back()
      );
      
      auto& dict = decl_dictionary::get();
      this->transformed.descriptors = decl_pair{
         .read = &dict.get_or_create_descriptor(this->transformed.variables.read),
         .save = &dict.get_or_create_descriptor(this->transformed.variables.save),
      };
   }
   
   /*virtual*/ expr_pair transform::generate(const instruction_generation_context& ctxt) const {
      auto& decl_dict = decl_dictionary::get();
      
      gw::expr::local_block block_read;
      gw::expr::local_block block_save;
      assert(!this->types.empty());
      
      auto statements_read = block_read.statements();
      auto statements_save = block_save.statements();
      
      value_pair trans = this->to_be_transformed_value.as_value_pair();
      //
      struct transform_step {
         decl_pair transformed_value;
         expr_pair transformed_value_decl;
         expr_pair transform_call;
      };
      std::vector<transform_step> steps;
      //
      for(size_t i = 0; i < types.size(); ++i) {
         auto  type = this->types[i];
         auto  name = type.name();
         auto& step = steps.emplace_back();
         
         gw::decl::variable var_read;
         gw::decl::variable var_save;
         if (i == this->types.size() - 1) {
            var_read = this->transformed.variables.read;
            var_save = this->transformed.variables.save;
         } else {
            auto name_read = lu::strings::printf_string("__transformed_read_as_%s", name.data());
            auto name_save = lu::strings::printf_string("__transformed_save_as_%s", name.data());
            var_read = gw::decl::variable(name_read.c_str(), type);
            var_save = gw::decl::variable(name_save.c_str(), type);
         }
         
         step.transformed_value = decl_pair{
            .read = &decl_dict.get_or_create_descriptor(var_read),
            .save = &decl_dict.get_or_create_descriptor(var_save),
         };
         step.transformed_value_decl = expr_pair{
            .read = var_read.make_declare_expr(),
            .save = var_save.make_declare_expr(),
         };
         //
         const auto& options = step.transformed_value.read->options.transform_options();
         step.transform_call = expr_pair{
            .read = gw::expr::call(
               options.post_unpack,
               // args:
               trans.read.address_of(), // in situ
               var_read.as_value().address_of() // transformed
            ),
            .save = gw::expr::call(
               options.pre_pack,
               // args:
               trans.read.address_of(), // in situ
               var_save.as_value().address_of() // transformed
            ),
         };
         
         trans.read = var_read.as_value();
         trans.save = var_save.as_value();
      }
      
      //
      // Transformations on read need to be in reverse order. Consider:
      //
      //    {
      //       struct TransformedMiddle __transformed_1;
      //       struct TransformedFinal  __transformed_2;
      //       PackTransform(&original, &__transformed_1);
      //       PackTransform(&__transformed_1, &__transformed_2);
      //       
      //       __child_instructions(&__transformed_2);
      //    }
      //    
      //    {
      //       struct TransformedMiddle __transformed_1;
      //       struct TransformedFinal  __transformed_2;
      //       
      //       __child_instructions(&__transformed_2);
      //       
      //       UnpackTransform(&__transformed_1, &__transformed_2);
      //       UnpackTransform(&original, &__transformed_1);
      //    }
      //
      
      // Append variable declarations.
      for(auto& step : steps) {
         statements_read.append(step.transformed_value_decl.read);
         statements_save.append(step.transformed_value_decl.save);
      }
      // Append pre-pack calls.
      for(auto& step : steps) {
         statements_save.append(step.transform_call.save);
      }
      // Append child instructions.
      for(auto& child_ptr : this->instructions) {
         auto pair = child_ptr->generate(ctxt);
         statements_read.append(pair.read);
         statements_save.append(pair.save);
      }
      // Append post-unpack calls.
      for(auto it = steps.rbegin(); it != steps.rend(); ++it) {
         auto& step = *it;
         statements_read.append(step.transform_call.read);
      }
      
      return expr_pair{
         .read = block_read,
         .save = block_save
      };
   }
}