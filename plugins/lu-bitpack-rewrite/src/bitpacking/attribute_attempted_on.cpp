#include "bitpacking/attribute_attempted_on.h"
#include <cassert>
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace bitpacking {
   extern bool attribute_attempted_on(gw::attribute_list list, lu::strings::zview attr_name) {
      for(auto attr : list) {
         const auto name = attr.name();
         if (name == attr_name) {
            return true;
         }
         if (name == "lu bitpack invalid attribute name") {
            auto data = attr.arguments().front().as_untyped();
            assert(data != NULL_TREE && TREE_CODE(data) == IDENTIFIER_NODE);
            if (std::string_view(IDENTIFIER_POINTER(data)) == attr_name) {
               return true;
            }
         }
      }
      return false;
   }
}