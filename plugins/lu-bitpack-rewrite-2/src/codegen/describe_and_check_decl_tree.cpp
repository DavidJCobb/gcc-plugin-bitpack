#include <memory> // GCC headers fuck up string-related STL identifiers that <memory> depends on
#include "codegen/describe_and_check_decl_tree.h"
#include "codegen/decl_descriptor.h"
#include "codegen/decl_dictionary.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace codegen {
   extern bool describe_and_check_decl_tree(const decl_descriptor& desc) {
      if (desc.has_any_errors)
         return false;
      if (desc.types.serialized->is_container()) {
         for(const auto* memb : desc.members_of_serialized())
            if (!describe_and_check_decl_tree(*memb))
               return false;
      }
      return true;
   }
   
   extern bool describe_and_check_decl_tree(gw::decl::variable decl) {
      auto& dict = decl_dictionary::get();
      auto& desc = dict.describe(decl);
      if (!describe_and_check_decl_tree(desc))
         return false;
      return true;
   }
}