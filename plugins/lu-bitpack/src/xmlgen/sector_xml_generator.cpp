#include "xmlgen/sector_xml_generator.h"
#include <charconv> // std::to_chars
#include "lu/strings/printf_string.h"
#include "bitpacking/heritable_options.h"
#include "codegen/descriptors.h"
#include "codegen/sector_functions_generator.h"
#include "codegen/serialization_value_path.h"

// For reporting the C types of the to-be-serialized top-level variables:
#include <gcc-plugin.h>
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier

#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/expr/floating_point_constant.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/record.h"
#include "gcc_wrappers/attribute.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace {
   using heritable_options           = bitpacking::heritable_options;
   using heritable_options_stockpile = bitpacking::heritable_options_stockpile;
   
   template<typename T>
   std::string _to_chars(T value) {
      char buffer[64] = { 0 };
      auto result = std::to_chars((char*)buffer, (char*)(&buffer + sizeof(buffer) - 1), value);
      if (result.ec == std::errc{}) {
         *result.ptr = '\0';
      } else {
         buffer[0] = '?';
         buffer[1] = '\0';
      }
      return buffer;
   }
}

namespace xmlgen {
   static void _set_default_value_info(xml_element& elem, gw::_wrapped_tree_node data) {
      if (auto casted = data.as<gw::expr::string_constant>(); !casted.empty()) {
         auto sub = std::make_unique<xml_element>();
         sub->node_name = "default-value-string";
         sub->text_content = casted.value_view();
         elem.append_child(std::move(sub));
      } else if (auto casted = data.as<gw::expr::integer_constant>(); !casted.empty()) {
         std::string text;
         {
            auto opt = casted.try_value_signed();
            if (opt.has_value()) {
               text = lu::strings::printf_string("%d", *opt);
            } else {
               text = "???";
            }
         }
         elem.set_attribute("default-value", text);
      } else if (auto casted = data.as<gw::expr::floating_point_constant>(); !casted.empty()) {
         auto text = casted.to_string();
         elem.set_attribute("default-value", text);
      } else if (!data.empty()) {
         elem.set_attribute("default-value", "???");
      }
   }
   
   std::unique_ptr<xml_element> sector_xml_generator::_make_whole_data_member(const codegen::member_descriptor& member) {
      auto ptr = std::make_unique<xml_element>();
      {
         auto name = member.decl.name();
         if (!name.empty())
            ptr->set_attribute("name", name);
      }
      {
         auto vt = member.value_type;
         ptr->set_attribute("c-type", vt.pretty_print());
         if (vt.is_record() && !vt.declaration().empty()) {
            ptr->set_attribute("c-type-defined-via-typedef", "true");
         }
      }
      {
         auto list = member.decl.attributes();
         auto attr = list.get_attribute("lu_bitpack_inherit");
         if (!attr.empty()) {
            //
            // If this attribute is present, we know it must be valid, or bitpacking 
            // would've failed and we wouldn't have anything to write out to XML. 
            // Don't bother doing any correctness checks now.
            //
            auto value = attr.arguments().front().as<gw::expr::string_constant>().value();
            ptr->set_attribute("inherit", value);
         }
         
         attr = list.get_attribute("lu_bitpack_default_value");
         if (!attr.empty()) {
            auto data = attr.arguments().front();
            _set_default_value_info(*ptr, data);
         }
      }
      const auto& computed = member.bitpacking_options;
      switch (member.kind) {
         case bitpacking::member_kind::boolean:
            ptr->node_name = "boolean";
            break;
         case bitpacking::member_kind::buffer:
            ptr->node_name = "opaque-buffer";
            {
               auto& options = computed.buffer_options();
               ptr->set_attribute("bytecount", _to_chars(options.bytecount));
            }
            break;
         case bitpacking::member_kind::integer:
            ptr->node_name = "integer";
            {
               auto& options = computed.integral_options();
               ptr->set_attribute("bitcount", _to_chars(options.bitcount));
               ptr->set_attribute("min",      _to_chars(options.min));
            }
            break;
         case bitpacking::member_kind::pointer:
            ptr->node_name = "pointer";
            {
               auto& options = computed.integral_options();
               ptr->set_attribute("bitcount", _to_chars(options.bitcount));
            }
            break;
         case bitpacking::member_kind::string:
            ptr->node_name = "string";
            {
               auto& options = computed.string_options();
               ptr->set_attribute("length", _to_chars(options.length));
               ptr->set_attribute("with-terminator", options.with_terminator ? "true" : "false");
            }
            break;
         case bitpacking::member_kind::structure:
            ptr->node_name = "struct";
            break;
         case bitpacking::member_kind::union_external_tag:
            ptr->node_name = "union";
            ptr->set_attribute("tag-type", "external");
            break;
         case bitpacking::member_kind::union_internal_tag:
            ptr->node_name = "union";
            ptr->set_attribute("tag-type", "internal");
            break;
         default:
            assert(false && "unreachable");
      }
      {
         auto& xfrm = member.bitpacking_options.transforms;
         if (auto& v = xfrm.pre_pack; !v.empty()) {
            ptr->set_attribute("pre-pack-transform", v.name());
         }
         if (auto& v = xfrm.post_unpack; !v.empty()) {
            ptr->set_attribute("post-unpack-transform", v.name());
         }
      }
      
      for(auto extent : member.array_extents) {
         auto rank_elem = std::make_unique<xml_element>();
         rank_elem->node_name = "array-rank";
         rank_elem->set_attribute("extent", _to_chars(extent));
         ptr->append_child(std::move(rank_elem));
      }
      
      return ptr;
   }
   
   void sector_xml_generator::_append_omitted_defaulted_members(xml_element& dst, const codegen::struct_descriptor& desc) {
      desc.type.for_each_referenceable_field([&dst](tree node) {
         auto decl = gw::decl::field::from_untyped(node);
         
         tree default_value = NULL_TREE;
         bool omitted       = false;
         {
            auto list = decl.value_type().attributes();
            if (list.has_attribute("lu_bitpack_omit")) {
               omitted = true;
               auto attr = list.get_attribute("lu_bitpack_default_value");
               if (!attr.empty())
                  default_value = attr.arguments().front().as_untyped();
            }
         }
         {
            auto list = decl.attributes();
            if (list.has_attribute("lu_bitpack_omit")) {
               omitted = true;
               auto attr = list.get_attribute("lu_bitpack_default_value");
               if (!attr.empty())
                  default_value = attr.arguments().front().as_untyped();
            }
         }
         if (!omitted || default_value == NULL_TREE)
            return;
         
         auto ptr = std::make_unique<xml_element>();
         ptr->node_name = "omitted";
         {
            auto name = decl.name();
            if (!name.empty())
               ptr->set_attribute("name", name);
         }
         {
            auto vt = decl.value_type();
            while (vt.is_array()) {
               std::string extent;
               {
                  auto opt = vt.as_array().extent();
                  if (opt.has_value())
                     extent = _to_chars(*opt);
               }
               auto rank_elem = std::make_unique<xml_element>();
               rank_elem->node_name = "array-rank";
               rank_elem->set_attribute("extent", extent);
               ptr->append_child(std::move(rank_elem));
               
               vt = vt.as_array().value_type();
            }
            ptr->set_attribute("c-type", vt.pretty_print());
            if (vt.is_record() && !vt.declaration().empty()) {
               ptr->set_attribute("c-type-defined-via-typedef", "true");
            }
         }
         gw::_wrapped_tree_node wrap;
         wrap.set_from_untyped(default_value);
         _set_default_value_info(*ptr, wrap);
         
         dst.append_child(std::move(ptr));
      });
   }
   
   void sector_xml_generator::_make_heritable_xml(std::string_view name, const heritable_options& options) {
      assert(!name.empty());
      
      auto ptr = std::make_unique<xml_element>();
      ptr->node_name = "heritable";
      ptr->set_attribute("name", name);
      
      if (auto* casted = std::get_if<heritable_options::integral_data>(&options.data)) {
         ptr->set_attribute("type", "integer");
         if (auto& opt = casted->bitcount; opt.has_value())
            ptr->set_attribute("bitcount", _to_chars(*opt));
         if (auto& opt = casted->min; opt.has_value())
            ptr->set_attribute("min", _to_chars(*opt));
         if (auto& opt = casted->max; opt.has_value())
            ptr->set_attribute("max", _to_chars(*opt));
      } else if (auto* casted = std::get_if<heritable_options::string_data>(&options.data)) {
         ptr->set_attribute("type", "string");
         if (auto& opt = casted->length; opt.has_value())
            ptr->set_attribute("length", _to_chars(*opt));
         if (auto& opt = casted->with_terminator; opt.has_value())
            ptr->set_attribute("with-terminator", *opt ? "true" : "false");
      }
      
      if (auto& v = options.transforms.pre_pack; !v.empty()) {
         ptr->set_attribute("pre-pack-transform", v);
      }
      if (auto& v = options.transforms.post_unpack; !v.empty()) {
         ptr->set_attribute("post-unpack-transform", v);
      }
      
      this->heritables[std::string(name)] = std::move(ptr);
   }

   void sector_xml_generator::run(const codegen::sector_functions_generator& gen) {
      heritable_options_stockpile::get().for_each_option([this](const std::string& name, const heritable_options& options) {
         this->_make_heritable_xml(name, options);
      });
      
      gen.for_each_seen_struct_descriptor([this](const codegen::struct_descriptor& descriptor) {
         auto root = std::make_unique<xml_element>();
         root->node_name = "struct-type";
         root->set_attribute("name",   descriptor.type.name());
         root->set_attribute("c-type", descriptor.type.pretty_print());
         {
            auto list = descriptor.type.attributes();
            auto attr = list.get_attribute("lu_bitpack_inherit");
            if (!attr.empty()) {
               //
               // If this attribute is present, we know it must be valid, or bitpacking 
               // would've failed and we wouldn't have anything to write out to XML. 
               // Don't bother doing any correctness checks now.
               //
               auto value = attr.arguments().front().as<gw::expr::string_constant>().value();
               root->set_attribute("inherit", value);
            }
         }
         
         for(auto& m_descriptor : descriptor.members) {
            root->append_child(std::move(_make_whole_data_member(m_descriptor)));
         }
         
         _append_omitted_defaulted_members(*root, descriptor);
         
         this->per_type_xml.push_back(std::move(root));
      });
      
      for(const auto& src_group : gen.identifiers_to_serialize) {
         auto& dst_group = this->serialized_identifiers.emplace_back();
         for(const auto& name : src_group) {
            auto node = lookup_name(get_identifier(name.c_str()));
            assert(node != NULL_TREE);
            assert(TREE_CODE(node) == VAR_DECL);
            
            auto decl = gw::decl::variable::from_untyped(node);
            auto type = decl.value_type();
            
            auto elem = std::make_unique<xml_element>();
            elem->node_name = "variable";
            elem->set_attribute("name",   name);
            elem->set_attribute("c-type", type.pretty_print());
            dst_group.push_back(std::move(elem));
         }
      }
      
      {
         auto& src_list = gen.reports.by_sector;
         auto& dst_list = this->per_sector_xml;
         auto  size     = src_list.size();
         dst_list.reserve(size);
         for(size_t i = 0; i < size; ++i) {
            const auto& report = src_list[i];
            
            auto root = std::make_unique<xml_element>();
            root->node_name = "sector";
            root->set_attribute("id", lu::strings::printf_string("%u", (int)i));
            
            for(const auto& path : report.serialized_object_paths) {
               auto ptr = std::make_unique<xml_element>();
               ptr->node_name = "value";
               ptr->set_attribute("path", path.path);
               root->append_child(std::move(ptr));
            }
            dst_list.push_back(std::move(root));
         }
      }
   }

   std::string sector_xml_generator::bake() const {
      std::string out = "<data>\n";
      {
         auto& list = this->heritables;
         if (!list.empty()) {
            out += "   <heritables>\n";
            for(const auto& pair : list) {
               out += pair.second->to_string(2);
               out += '\n';
            }
            out += "   </heritables>\n";
         }
      }
      {
         auto& list = this->per_type_xml;
         if (!list.empty()) {
            out += "   <types>\n";
            for(const auto& node : list) {
               out += node->to_string(2);
               out += '\n';
            }
            out += "   </types>\n";
         }
      }
      {
         auto& list_outer = this->serialized_identifiers;
         if (!list_outer.empty()) {
            out += "   <variables>\n";
            for(const auto& list_inner : list_outer) {
               if (list_inner.empty()) {
                  out += "      <group />\n";
                  continue;
               }
               out += "      <group>\n";
               for(const auto& node : list_inner) {
                  out += node->to_string(3);
                  out += '\n';
               }
               out += "      </group>\n";
            }
            out += "   </variables>\n";
         }
      }
      {
         auto& list = this->per_sector_xml;
         if (!list.empty()) {
            out += "   <sectors>\n";
            for(const auto& item : list) {
               out += item->to_string(2);
               out += '\n';
            }
            out += "   </sectors>\n";
         }
      }
      out += "</data>";
      return out;
   }
}