#include "bitpacking/get_union_bitpacking_info.h"
#include "bitpacking/for_each_influencing_entity.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/attribute.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace bitpacking {
   static void _check_attributes(
      gw::_wrapped_tree_node node,
      union_bitpacking_info& info,
      gw::attribute_list     list
   ) {
      bool node_is_decl = node.is<gw::decl::base>();
      for(auto attr : list) {
         auto name = attr.name();
         if (name == "lu_bitpack_union_external_tag") {
            auto data = attr.arguments().front().as_untyped();
            assert(data != NULL_TREE && TREE_CODE(data) == IDENTIFIER_NODE);
            
            auto& opt = info.external;
            auto& dst = opt.has_value() ? *opt : opt.emplace();
            if (dst.specifying_node.empty() || node_is_decl) {
               dst.identifier      = IDENTIFIER_POINTER(data);
               dst.specifying_node = node;
            }
            continue;
         }
         if (name == "lu_bitpack_union_internal_tag") {
            auto data = attr.arguments().front().as_untyped();
            assert(data != NULL_TREE && TREE_CODE(data) == IDENTIFIER_NODE);
            
            auto& opt = info.internal;
            auto& dst = opt.has_value() ? *opt : opt.emplace();
            if (dst.specifying_node.empty() || node_is_decl) {
               dst.identifier      = IDENTIFIER_POINTER(data);
               dst.specifying_node = node;
            }
            continue;
         }
         if (name == "lu bitpack invalid attribute name") {
            auto data = attr.arguments().front().as_untyped();
            assert(data != NULL_TREE && TREE_CODE(data) == IDENTIFIER_NODE);
            name = IDENTIFIER_POINTER(data);
            if (name == "lu_bitpack_union_external_tag") {
               auto& opt = info.external;
               auto& dst = opt.has_value() ? *opt : opt.emplace();
               if (dst.specifying_node.empty() || node_is_decl) {
                  dst.specifying_node = node;
               }
               continue;
            }
            if (name == "lu_bitpack_union_internal_tag") {
               auto& opt = info.internal;
               auto& dst = opt.has_value() ? *opt : opt.emplace();
               if (dst.specifying_node.empty() || node_is_decl) {
                  dst.specifying_node = node;
               }
               continue;
            }
         }
      }
   }
   
   extern union_bitpacking_info get_union_bitpacking_info(
      gw::decl::field decl,
      gw::type::base  type
   ) {
      union_bitpacking_info out;
      
      if (!decl.empty()) {
         _check_attributes(decl, out, decl.attributes());
         if (type.empty()) {
            type = decl.value_type();
         }
      }
      if (!type.empty()) {
         bitpacking::for_each_influencing_entity(type, [&out](tree node) {
            gw::attribute_list list;
            if (TYPE_P(node)) {
               list = gw::type::base::from_untyped(node).attributes();
            } else {
               list = gw::decl::base::from_untyped(node).attributes();
            }
            
            gw::_wrapped_tree_node wrap;
            wrap.set_from_untyped(node);
            _check_attributes(wrap, out, list);
         });
      }
      
      return out;
   }
}