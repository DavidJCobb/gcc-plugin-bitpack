#include "codegen/serializable_object.h"

namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace codegen {
   
   //
   // serializable_object
   //
   
   /*static*/ std::unique_ptr<serializable_object> serializable_object::_from_field(
      const stream_function_set& global_options,
      gcc_wrappers::decl::field  decl
   ) {
      //
      // Check whether this member is skipped.
      //
      for(auto it : decl.attributes()) {
         auto k = it->first;
         if (TREE_CODE(k) != IDENTIFIER_NODE)
            continue;
         std::string_view key = IDENTIFIER_POINTER(k);
         if (key == "lu_bitpack_omit")
            return {};
      }
      
      auto value_type = decl.value_type();
      
      if (decl.name().empty()) {
         //
         // Anonymous struct/union member.
         //
         assert(value_type.is_record() || value_type.is_union());
         static_assert(false, "TODO");
      }
      
      auto  ptr = std::make_unique<serializable_object>(dictionary);
      auto& out = *ptr;
      
      out.type = value_type;
      if (value_type.is_array()) {
         auto avt = value_type.array_value_type();
         if (avt == global_options.types.string_char) {
            out.serialization_type = type_code::string;
         } else {
            out.serialization_type = type_code::array;
            static_assert(false, "TODO: Generate child-element serializable_objects.");
         }
      } else {
         if (value_type.is_record()) {
            out.serialization_type = type_code::structure;
         } else {
            if (value_type == global_options.types.boolean) {
               out.serialization_type = type_code::boolean;
            }
            static_assert(false, "TODO");
         }
      }
      
      out.type       = vt;
      out.value_type = vt;
      while (out.value_type.is_array()) {
         out.value_type = out.value_type.array_value_type();
      }
      
      static_assert(false, "TODO: serialization_type");
      
      return ptr;
   }
   
   /*static*/ std::unique_ptr<serializable_object> serializable_object::from_type(
      const stream_function_set& global_options,
      gcc_wrappers::type            type
   ) {
      assert(type.is_record());
      
      auto  ptr = std::make_unique<serializable_object>(dictionary);
      auto& out = *ptr;
   
      out.serialization_type = type_code::structure;
      out.type       = type;
      out.value_type = type;
      
      auto& ctnr = out.container_info.emplace();
      type.for_each_field([&out, &ctnr](tree raw_decl) {
         auto decl = gcc_wrappers::decl::field::from_untyped(raw_decl);
         auto sub  = _from_field(dictionary, decl);
         if (!sub) {
            return;
         }
         
         auto& sub_data = sub->subobject_info.emplace();
         sub_data.container = &out;
         sub_data.decl      = decl;
         
         ctnr.push_back(std::move(sub));
         
         /*//
         auto attr = decl.attributes();
         bool skip = false;
         attr.for_each_kv_pair([&skip](tree k, tree v) -> bool {
            if (TREE_CODE(k) != IDENTIFIER_NODE)
               return true;
            std::string_view key = IDENTIFIER_POINTER(k);
            if (key == "lu_bitpack_omit") {
               skip = true;
               return false;
            }
            return true;
         };
         if (skip)
            continue;
         
         auto& member = this->members.emplace_back(decl);
         member.recompute_options();
         //*/
      });
      
      return ptr;
   }
}