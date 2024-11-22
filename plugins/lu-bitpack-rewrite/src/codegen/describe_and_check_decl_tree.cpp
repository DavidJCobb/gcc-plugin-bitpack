#include "codegen/describe_and_check_decl_tree.h"
#include "codegen/decl_descriptor.h"

namespace codegen {
   extern bool describe_and_check_decl_tree(const decl_descriptor& desc) {
      if (desc.has_any_errors)
         return false;
      auto type = desc.types.serialized;
      if (type.is_record() || type.is_union()) {
         for(const auto* memb : desc.members_of_serialized())
            if (!describe_and_check_decl_tree(*memb))
               return false;
      }
      return true;
   }
   
   extern bool describe_and_check_decl_tree(gcc_wrappers::decl::variable decl) {
      assert(!decl.empty());
      auto& dict = decl_dictionary::get();
      auto& desc = dict.get_or_create_descriptor(decl);
      if (!describe_and_check_decl_tree(desc))
         return false;
      return true;
   }
}