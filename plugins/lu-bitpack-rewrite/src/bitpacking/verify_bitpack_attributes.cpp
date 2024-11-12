#include "bitpacking/verify_bitpack_attributes.h"
#include "bitpacking/attribute_attempted_on.h"
#include "bitpacking/for_each_influencing_entity.h"
#include "bitpacking/mark_for_invalid_attributes.h"
#include "lu/strings/zview.h"
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/attribute.h"
#include "bitpacking/get_union_bitpacking_info.h"
#include "bitpacking/verify_union_external_tag.h"
#include "bitpacking/verify_union_internal_tag.h"
#include "bitpacking/verify_union_members.h"
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

// Requirements for general container members:
static void _disallow_transforming_non_addressable(gw::decl::field decl) {
   //
   // Non-addressable fields shouldn't be allowed to use transform options. 
   // We can only check this on the type, because as of GCC 11.4.0, the 
   // "decl finished" plug-in callback fires before attributes are attached; 
   // we have to react to the containing type being finished.
   //
   if (_attr_present_or_attempted(decl, "lu_bitpack_transforms")) {
      error_at(
         decl.source_location(),
         "attribute %<lu_bitpack_transform%> (applied to field %<%s%>) can only be applied to objects that support having their addresses taken (e.g. not bit-fields)",
         decl.name().data()
      );
      bitpacking::mark_for_invalid_attributes(decl);
   }
}
static void _disallow_contradictory_union_tagging(
   gw::decl::field decl,
   const bitpacking::union_bitpacking_info& info
) {
   if (!info.external.has_value() || !info.internal.has_value())
      return;
   
   error_at(decl.source_location(), "attributes %<lu_bitpack_union_external_tag%> and %<lu_bitpack_union_internal_tag%> are mutually exclusive but both affect this object");
   bitpacking::mark_for_invalid_attributes(decl);
   
   location_t loc_ext = UNKNOWN_LOCATION;
   location_t loc_int = UNKNOWN_LOCATION;
   {
      auto node = (*info.external).specifying_node;
      if (node.is<gw::type::base>())
         loc_ext = node.as<gw::type::base>().source_location();
      else if (node.is<gw::decl::base>())
         loc_ext = node.as<gw::decl::base>().source_location();
   }
   {
      auto node = (*info.internal).specifying_node;
      if (node.is<gw::type::base>())
         loc_int = node.as<gw::type::base>().source_location();
      else if (node.is<gw::decl::base>())
         loc_int = node.as<gw::decl::base>().source_location();
   }
   if (loc_ext != UNKNOWN_LOCATION) {
      inform(loc_ext, "the object is externally tagged as a result of attributes applied here");
   }
   if (loc_ext != UNKNOWN_LOCATION) {
      inform(loc_int, "the object is internally tagged as a result of attributes applied here");
   }
}

// Requirements for union members:
static void _disallow_union_member_attributes_on_non_union_member(gw::decl::field decl) {
   //
   // The `lu_bitpack_union_member_id` attribute is only permitted on FIELD_DECLs 
   // of a UNION_TYPE. We can only check this when the type is finished, because 
   // DECL_CONTEXT isn't set on a FIELD_DECL at the time that its attributes are 
   // being processed and applied.
   //
   if (bitpacking::attribute_attempted_on(decl.attributes(), "lu_bitpack_union_member_id")) {
      error_at(
         decl.source_location(),
         "attribute %<lu_bitpack_union_member_id%> (applied to field %<%s%>) can only be applied to data members in a union type",
         decl.name().data()
      );
      bitpacking::mark_for_invalid_attributes(decl);
   }
}

namespace bitpacking { 
   extern void verify_bitpack_attributes(tree node) {
      if (TREE_CODE(node) == RECORD_TYPE || TREE_CODE(node) == UNION_TYPE) {
         auto type     = gw::type::container::from_untyped(node);
         bool is_union = type.is_union();
         if (is_union) {
            //
            // We can't mark types with an error sentinel after they're finished, 
            // and attribute handlers applied to a type always see the type as 
            // incomplete and therefore can't validate its contents. However, 
            // this is still the best place to do error checking and reporting.
            //
            auto info = get_union_bitpacking_info({}, type);
            if (!info.empty()) {
               verify_union_members(type.as_union());
               if (info.internal.has_value()) {
                  verify_union_internal_tag(type.as_union(), (*info.internal).identifier);
               }
            }
         }
         type.for_each_referenceable_field([is_union](gw::decl::field decl) {
            if (decl.is_non_addressable()) {
               _disallow_transforming_non_addressable(decl);
            }
            
            // Validate external unions' contexts when those context are 
            // available.
            auto info = get_union_bitpacking_info(decl, {});
            if (!info.empty()) {
               auto decl_type = decl.value_type();
               while (decl_type.is_array())
                  decl_type = decl_type.as_array().value_type();
               if (!decl_type.is_union()) {
                  error_at(
                     decl.source_location(),
                     "attributes %<lu_bitpack_union_external_tag%> and %<lu_bitpack_union_internal_tag%> can only be applied to unions or arrays of unions"
                  );
                  bitpacking::mark_for_invalid_attributes(decl);
               }
               
               if (is_union && info.external.has_value()) {
                  error_at(
                     decl.source_location(),
                     "attribute %<lu_bitpack_union_external_tag%> (applied to field %<%s%>) can only be applied to union-type data members in a struct type; this is not a member of a struct",
                     decl.name().data()
                  );
                  bitpacking::mark_for_invalid_attributes(decl);
               }
               
               _disallow_contradictory_union_tagging(decl, info);
               
               if (info.external.has_value()) {
                  if (!verify_union_external_tag(decl, (*info.external).identifier)) {
                     bitpacking::mark_for_invalid_attributes(decl);
                  }
               }
            }
            
            // Validate that `lu_bitpack_union_member_id` has been used on a union member.
            if (!is_union) {
               _disallow_union_member_attributes_on_non_union_member(decl);
            }
         });
      }
   }
}