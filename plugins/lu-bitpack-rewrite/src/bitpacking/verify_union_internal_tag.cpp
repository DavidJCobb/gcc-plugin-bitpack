#include "bitpacking/verify_union_external_tag.h"
#include <string>
#include "gcc_wrappers/type/record.h"
#include "gcc_wrappers/type/untagged_union.h"
#include <diagnostic.h>
namespace gw {
   using namespace gcc_wrappers;
}

namespace bitpacking {
   // Return `false` if invalid.
   extern bool verify_union_internal_tag(
      gw::type::untagged_union type,
      lu::strings::zview       tag_identifier,
      bool silent
   ) {
      if (tag_identifier.empty())
         return false;
      
      bool valid = true;
      
      std::vector<gw::decl::field> matched_members;
      {
         bool is_first = true;
         type.for_each_field([silent, &matched_members, &valid, &is_first](gw::decl::field decl) {
            auto decl_name  = decl.name();
            auto value_type = decl.value_type();
            {
               bool omitted = false;
               for(auto attr : decl.attributes()) {
                  auto name = attr.name();
                  if (name == "lu_bitpack_omit") {
                     omitted = true;
                     break;
                  }
                  if (name == "lu bitpack invalid attribute name") {
                     auto node = attr.arguments().front().as_untyped();
                     assert(node != NULL_TREE && TREE_CODE(node) == IDENTIFIER_NODE);
                     name = IDENTIFIER_POINTER(node);
                     
                     if (name == "lu_bitpack_omit") {
                        omitted = true;
                        break;
                     }
                  }
               }
               if (omitted) {
                  return;
               }
            }
            if (!value_type.is_record()) {
               if (!silent) {
                  if (decl_name.empty()) {
                     error_at(decl.source_location(), "%<lu_bitpack_union_internal_tag%>: all non-omitted members of an internally tagged union must be struct types; this unnamed member is not");
                  } else {
                     error_at(decl.source_location(), "%<lu_bitpack_union_internal_tag%>: all non-omitted members of an internally tagged union must be struct types; member %<%s%> is not", decl.name().data());
                  }
               }
               is_first = false;
               valid    = false;
               return;
            }
            auto record_type = value_type.as_record();
            if (is_first) {
               record_type.for_each_referenceable_field([&matched_members](gw::decl::field nest) {
                  matched_members.push_back(nest);
               });
               is_first = false;
            } else {
               size_t i = 0;
               record_type.for_each_referenceable_field([&matched_members, &i](gw::decl::field field) {
                  if (i >= matched_members.size()) {
                     return false;
                  }
                  auto a  = field;
                  auto b  = matched_members[i];
                  bool eq = [&a, &b]() -> bool {
                     if (a.name() != b.name())
                        return false;
                     if (a.value_type() != b.value_type())
                        return false;
                     return true;
                  }();
                  if (!eq) {
                     matched_members.resize(i);
                     return false;
                  }
                  
                  ++i;
                  return true;
               });
            }
         });
      }
      
      if (matched_members.empty()) {
         if (!silent) {
            auto name = type.name();
            if (name.empty()) {
               error_at(type.source_location(), "%<lu_bitpack_union_internal_tag%>: the union%'s non-omitted members must all be struct types, those structs%' first field(s) must be identical, and the union%'s tag must be one of those fields. none of the structs have any first fields in common");
            } else {
               error_at(type.source_location(), "%<lu_bitpack_union_internal_tag%>: the non-omitted members of union type %<%s%> must all be struct types, those structs%' first field(s) must be identical, and the union%'s tag must be one of those fields. none of the structs have any first fields in common", name.data());
            }
         }
         return false;
      }
      bool found = false;
      for(auto decl : matched_members) {
         if (decl.name() == tag_identifier) {
            found = true;
            break;
         }
      }
      if (!found) {
         if (!silent) {
            auto name = type.name();
            if (name.empty()) {
               error_at(type.source_location(), "%<lu_bitpack_union_internal_tag%>: the union%'s non-omitted members must all be struct types, those structs%' first field(s) must be identical, and the union%'s tag must be one of those fields. tag identifier %<%s%> does not refer to any of those fields", tag_identifier.c_str());
            } else {
               error_at(type.source_location(), "%<lu_bitpack_union_internal_tag%>: the non-omitted members of union type %<%s%> must all be struct types, those structs%' first field(s) must be identical, and the union%'s tag must be one of those fields. tag identifier %<%s%> does not refer to any of those fields", name.data(), tag_identifier.c_str());
            }
            if (matched_members.size() == 1) {
               inform(type.source_location(), "the only common field between those structs is %<%s%>", matched_members[0].name().data());
            } else {
               inform(
                  type.source_location(),
                  "those structs have their first %u fields (from %<%s%> through %<%s%>) in common",
                  (int)matched_members.size(),
                  matched_members.front().name().data(),
                  matched_members.back().name().data()
               );
            }
         }
         return false;
      }
      
      return valid;
   }
}