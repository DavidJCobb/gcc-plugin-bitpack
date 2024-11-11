#include "bitpacking/verify_bitpack_attributes.h"
#include "bitpacking/attribute_attempted_on.h"
#include "bitpacking/for_each_influencing_entity.h"
#include "bitpacking/mark_for_invalid_attributes.h"
#include "lu/strings/zview.h"
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/attribute.h"
#include <stringpool.h> // get_identifier; dependency for <attribs.h>
#include <attribs.h> // decl_attributes
#include <c-family/c-common.h>
#include <diagnostic.h>
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

static bool _attr_present_or_attempted(gw::decl::field decl, lu::strings::zview attr_name) {
   bool present = false;
   bitpacking::for_each_influencing_entity(decl, [attr_name, &present](tree node) -> bool {
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
static bool _attr_present_or_attempted(gw::type::base type, lu::strings::zview attr_name) {
   bool present = false;
   bitpacking::for_each_influencing_entity(type, [attr_name, &present](tree node) -> bool {
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

static void _disallow_transforming_non_addressable(gw::decl::field decl) {
   if (_attr_present_or_attempted(decl, "lu_bitpack_transforms")) {
      error_at(
         decl.source_location(),
         "attribute %<lu_bitpack_transform%> (applied to field %<%s%>) can only be applied to objects that support having their addresses taken (e.g. not bit-fields)",
         decl.name().data()
      );
      bitpacking::mark_for_invalid_attributes(decl);
   }
}

namespace bitpacking { 
   extern void verify_bitpack_attributes(tree node) {
      if (TREE_CODE(node) == RECORD_TYPE || TREE_CODE(node) == UNION_TYPE) {
         auto type = gw::type::container::from_untyped(node);
         //
         // Non-addressable fields shouldn't be allowed to use transform options. 
         // We can only check this on the type, because as of GCC 11.4.0, the 
         // "decl finished" plug-in callback fires before attributes are attached; 
         // we have to react to the containing type being finished.
         //
         type.for_each_referenceable_field([](tree raw) {
            auto decl = gw::decl::field::from_untyped(raw);
            if (decl.is_non_addressable()) {
               _disallow_transforming_non_addressable(decl);
            }
         });
      }
   }
}