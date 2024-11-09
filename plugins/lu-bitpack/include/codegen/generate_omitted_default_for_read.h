#pragma once
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/type/record.h"

namespace codegen::generate_omitted_default_for_read {
   extern bool is_omitted(gcc_wrappers::decl::field);
   extern bool has_omitted_members(gcc_wrappers::type::record);
   
   extern tree default_value_of(gcc_wrappers::decl::field);
   
   // Call in two situations:
   //
   //  - When generating the whole-struct serialization function for a struct type, 
   //    to generate assign-to-default instructions for omitted-and-defaulted members 
   //    of that struct.
   //
   //  - When a struct doesn't fit in the current sector and needs to bes serialized 
   //    one member at a time, rather than by calling the whole-struct function.
   //
   // If a nested struct contains omitted-and-defaulted members but is not itself 
   // omitted, then we don't generate code for it. This is on the assumptiont that 
   // the caller (`sector_functions_generator`) will see that nested struct, which 
   // would lead to one of the two above cases for the nested struct.
   //
   extern gcc_wrappers::expr::base generate_for_members(gcc_wrappers::value struct_instance);
}