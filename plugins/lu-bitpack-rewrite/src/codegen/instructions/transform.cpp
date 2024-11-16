#include "codegen/instructions/transform.h"

namespace codegen::instructions {
   transform::transform(const std::vector<gcc_wrappers::type::base>& types) {
      this->types = types;
      assert(!types.empty());
      
      this->transformed.variables.read = gcc_wrappers::decl::variable(
         "__transformed_read",
         types.back()
      );
      this->transformed.variables.save = gcc_wrappers::decl::variable(
         "__transformed_save",
         types.back()
      );
      
      auto& dict = decl_dictionary::get();
      this->transformed.descriptors = decl_pair{
         .read = &dict.get_or_create_descriptor(this->transformed.variables.read),
         .save = &dict.get_or_create_descriptor(this->transformed.variables.save),
      };
   }
}