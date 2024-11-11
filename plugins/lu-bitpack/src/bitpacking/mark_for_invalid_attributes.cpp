#include "bitpacking/mark_for_invalid_attributes.h"
#include <stringpool.h> // dependency for <attribs.h>
#include <attribs.h> // decl_attributes
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
   
   constexpr const char* const sentinel_name = "lu bitpack invalid attributes";
}

namespace bitpacking {
   extern void mark_for_invalid_attributes(gcc_wrappers::decl::base decl) {
      if (decl.attributes().has_attribute(sentinel_name))
         return;
      tree node = decl.as_untyped();
      tree attr = tree_cons(get_identifier(sentinel_name), NULL_TREE, NULL_TREE);
      decl_attributes(
         &node,
         attr,
         ATTR_FLAG_INTERNAL,
         NULL_TREE
      );
   }
   extern void mark_for_invalid_attributes(gcc_wrappers::type::base type) {
      if (type.attributes().has_attribute(sentinel_name))
         return;
      tree node = type.as_untyped();
      tree attr = tree_cons(get_identifier(sentinel_name), NULL_TREE, NULL_TREE);
      decl_attributes(
         &node,
         attr,
         ATTR_FLAG_INTERNAL,
         NULL_TREE
      );
   }
}