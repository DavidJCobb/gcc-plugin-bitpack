#include "codegen/instructions/array_slice.h"
#include "gcc_wrappers/builtin_types.h"

namespace codegen::instructions {
   array_slice::array_slice() {
      const auto& ty = gcc_wrappers::builtin_types::get();
      
      this->loop_index.variables.read = gcc_wrappers::decl::variable(
         "__i_read",
         ty.basic_int
      );
      this->loop_index.variables.save = gcc_wrappers::decl::variable(
         "__i_save",
         ty.basic_int
      );
       
      auto& dict = decl_dictionary::get();
      this->loop_index.descriptors = decl_pair{
         .read = &dict.get_or_create_descriptor(this->loop_index.variables.read),
         .save = &dict.get_or_create_descriptor(this->loop_index.variables.save),
      };
   }
}