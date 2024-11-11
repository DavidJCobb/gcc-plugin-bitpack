#include "bitpacking/verify_bitpack_attributes.h"
#include "bitpacking/attribute_attempted_on.h"
#include "bitpacking/for_each_influencing_entity.h"
#include "bitpacking/mark_for_invalid_attributes.h"
#include "lu/strings/zview.h"
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/attribute.h"
#include <stringpool.h> // get_identifier; dependency for <attribs.h>
#include <attribs.h> // decl_attributes
#include <diagnostic.h>
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

static bool _attr_present_or_attempted(gw::decl::field decl, lu::strings::zview attr_name) {
   bool present = false;
   bitpacking::for_each_influencing_entity(decl, [attr_name, &decl, &present](tree node) -> bool {
      gw::attribute_list list;
      if (TYPE_P(node)) {
         list = gw::type::base::from_untyped(node).attributes();
      } else {
         list = gw::decl::base::from_untyped(node).attributes();
      }
      if (bitpacking::attribute_attempted_on(list, attr_name)) {
         present = true;
         return false;
      }
      return true;
   });
   return present;
}

//
// The transform options for bitpacking require us to be able to 
// take the address of a value. You can't take the address of a 
// C bitfield.
//
static void _disallow_transforming_bitfields(gw::decl::field decl) {
   if (_attr_present_or_attempted(decl, "lu_bitpack_transform")) {
      error_at(
         decl.source_location(),
         "attribute %<lu_bitpack_transform%> can only be applied to objects that support having their addresses taken (i.e. C bit-fields are not compatible)"
      );
      bitpacking::mark_for_invalid_attributes(decl);
   }
}

namespace bitpacking {
   extern void verify_bitpack_attributes(tree node) {
      if (TREE_CODE(node) == FIELD_DECL) {
         auto decl = gw::decl::field::from_untyped(node);
         if (decl.is_bitfield()) {
            _disallow_transforming_bitfields(decl);
         }
      }
   }
}