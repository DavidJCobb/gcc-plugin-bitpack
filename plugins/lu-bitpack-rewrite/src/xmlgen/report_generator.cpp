#include "xmlgen/report_generator.h"
#include <cassert>
#include <limits>
#include "lu/strings/printf_string.h"
#include "bitpacking/data_options/computed.h"
#include "bitpacking/global_options.h"
#include "basic_global_state.h"
#include "codegen/decl_descriptor.h"
#include "codegen/stats_gatherer.h"
#include "codegen/whole_struct_function_dictionary.h"
#include "codegen/whole_struct_function_info.h"
#include "gcc_wrappers/decl/param.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/expr/floating_point_constant.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/type/record.h"
#include "gcc_wrappers/type/untagged_union.h"
namespace gw {
   using namespace gcc_wrappers;
}

#include "codegen/instructions/base.h"
#include "codegen/instructions/array_slice.h"
#include "codegen/instructions/padding.h"
#include "codegen/instructions/single.h"
#include "codegen/instructions/transform.h"
#include "codegen/instructions/union_switch.h"
#include "codegen/walk_instruction_nodes.h"

namespace {
   using owned_element = xmlgen::report_generator::owned_element;
}
static std::string _to_string(int v) {
   return lu::strings::printf_string("%d", v);
}

namespace xmlgen {
   report::stats& report_generator::_get_or_create_category_info(std::string_view name) {
      for(auto& pair : this->_categories)
         if (pair.first == name)
            return pair.second;
      auto& pair = this->_categories.emplace_back();
      pair.first = name;
      return pair.second;
   }
   report::c_type& report_generator::_get_or_create_type_info(gw::type::base type) {
      for(auto& info : this->_types)
         if (info.type.as_untyped() == type.as_untyped())
            return info;
      auto& info = this->_types.emplace_back(type);
      this->_apply_type_info_to(*info.as_xml, type);
      return info;
   }
   
   void report_generator::_apply_x_options_to(
      xml_element& node,
      const bitpacking::data_options::computed& options
   ) {
      if (options.is_buffer()) {
         const auto& casted = options.buffer_options();
         node.set_attribute("bytecount", _to_string(casted.bytecount));
      } else if (options.is_integral()) {
         const auto& casted = options.integral_options();
         node.set_attribute("bitcount", _to_string(casted.bitcount));
         if (options.kind != bitpacking::member_kind::pointer) {
            node.set_attribute("min", _to_string(casted.min));
         }
      } else if (options.is_string()) {
         const auto& casted = options.string_options();
         node.set_attribute("length",    _to_string(casted.length));
         node.set_attribute("nonstring", casted.nonstring ? "true" : "false");
      } else if (options.is_tagged_union()) {
         const auto& casted = options.tagged_union_options();
         node.set_attribute("tag", casted.tag_identifier);
      } else if (options.is_transforms()) {
         const auto& casted = options.transform_options();
         node.set_attribute("transformed-type", casted.transformed_type.pretty_print());
         node.set_attribute("pack-function", casted.pre_pack.name());
         node.set_attribute("unpack-function", casted.post_unpack.name());
      }
   }
   
   void report_generator::_apply_type_info_to(xml_element& node, gw::type::base type) {
      // Options.
      bitpacking::data_options::computed options;
      options.load(type);
      {
         std::unique_ptr<xml_element> x_opt;
         if (options.is_buffer()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "opaque-buffer-options";
         } else if (options.is_integral()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "integral-options";
         } else if (options.is_string()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "string-options";
         } else if (options.is_tagged_union()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "union-options";
         } else if (options.is_transforms()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "transform-options";
         }
         if (x_opt != nullptr) {
            _apply_x_options_to(*x_opt, options);
            node.append_child(std::move(x_opt));
         }
      }
   }
   
   std::string report_generator::_loop_variable_to_string(codegen::decl_pair pair) const {
      const auto& list = this->_loop_variables;
      if (list.size() <= 26) {
         for(size_t i = 0; i < list.size(); ++i) {
            if (list[i] != pair)
               continue;
            std::string name = "__a";
            name[2] += i;
            return name;
         }
      } else {
         for(size_t i = 0; i < list.size(); ++i)
            if (list[i] == pair)
               return lu::strings::printf_string("__loop_var_%u", (int)i);
      }
      return "__unknown_var";
   }
   std::string report_generator::_variable_to_string(codegen::decl_pair pair) const {
      assert(!pair.read->decl.empty());
      
      for(size_t i = 0; i < this->_transformed_values.size(); ++i)
         if (pair == this->_transformed_values[i])
            return lu::strings::printf_string("__transformed_var_%u", (int)i);
      
      return std::string(pair.read->decl.name());
   }
   std::string report_generator::_value_path_to_string(const codegen::value_path& path) const {
      std::string out;
      bool first = true;
      for(size_t i = 0; i < path.segments.size(); ++i) {
         const auto& segm = path.segments[i];
         const auto  pair = segm.descriptor();
         
         if (i == 0) {
            assert(!segm.is_array_access());
            auto decl = pair.read->decl;
            assert(!decl.empty());
            if (decl.is<gw::decl::param>()) {
               //
               // This is a whole-struct serialization function. Do not emit 
               // the name of the struct pointer parameter.
               //
               continue;
            }
         }
         
         if (segm.is_array_access()) {
            if (std::holds_alternative<size_t>(segm.data)) {
               out += lu::strings::printf_string("[%u]", (int)std::get<size_t>(segm.data));
               continue;
            }
            out += '[';
            assert(!pair.read->decl.empty());
            out += _loop_variable_to_string(pair);
            out += ']';
            continue;
         }
         
         if (!first)
            out += '.';
         first = false;
         out += _variable_to_string(pair);
      }
      return out;
   }
   
   owned_element report_generator::_generate(const codegen::instructions::array_slice& instr) {
      auto  node_ptr = std::make_unique<xml_element>();
      auto& node     = *node_ptr;
      
      node.node_name = "loop";
      node.set_attribute("array", _value_path_to_string(instr.array.value));
      node.set_attribute("start", _to_string(instr.array.start));
      node.set_attribute("count", _to_string(instr.array.count));
      node.set_attribute("counter-var", _loop_variable_to_string(instr.loop_index.descriptors));
      
      for(auto& child_ptr : instr.instructions) {
         node.append_child(_generate(*child_ptr));
      }
      
      return node_ptr;
   }
   owned_element report_generator::_generate(const codegen::instructions::padding& instr) {
      auto  node_ptr = std::make_unique<xml_element>();
      auto& node     = *node_ptr;
      
      node.node_name = "padding";
      node.set_attribute("bitcount", _to_string(instr.bitcount));
      
      return node_ptr;
   }
   owned_element report_generator::_generate(const codegen::instructions::single& instr) {
      auto  node_ptr = std::make_unique<xml_element>();
      auto& node     = *node_ptr;
      
      node.set_attribute("value", _value_path_to_string(instr.value));
      
      codegen::decl_pair desc_pair;
      for(auto& segm : instr.value.segments) {
         if (segm.is_array_access())
            continue;
         desc_pair = segm.member_descriptor();
      }
      assert(desc_pair.read != nullptr);
      auto value_type = desc_pair.read->types.serialized;
      node.set_attribute("type", value_type.pretty_print());
      
      const auto& options = instr.value.bitpacking_options();
      if (options.default_value_node != NULL_TREE) {
         gw::_wrapped_tree_node data;
         data.set_from_untyped(options.default_value_node);
         
         if (auto casted = data.as<gw::expr::string_constant>(); !casted.empty()) {
            auto sub = std::make_unique<xml_element>();
            sub->node_name = "default-value-string";
            sub->text_content = casted.value_view();
            node.append_child(std::move(sub));
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
            node.set_attribute("default-value", text);
         } else if (auto casted = data.as<gw::expr::floating_point_constant>(); !casted.empty()) {
            auto text = casted.to_string();
            node.set_attribute("default-value", text);
         } else if (!data.empty()) {
            node.set_attribute("default-value", "???");
         }
      }
      if (options.omit_from_bitpacking) {
         node.node_name = "omitted";
         return node_ptr;
      }
      
      node.node_name = "unknown";
      switch (options.kind) {
         case bitpacking::member_kind::boolean:
            node.node_name = "boolean";
            break;
         case bitpacking::member_kind::buffer:
            node.node_name = "buffer";
            break;
         case bitpacking::member_kind::integer:
            node.node_name = "integer";
            break;
         case bitpacking::member_kind::pointer:
            node.node_name = "pointer";
            break;
         case bitpacking::member_kind::string:
            node.node_name = "string";
            break;
         case bitpacking::member_kind::structure:
            node.node_name = "structure";
            break;
         case bitpacking::member_kind::transformed:
            node.node_name = "transformed";
            break;
         case bitpacking::member_kind::union_external_tag:
            node.node_name = "union-external-tag";
            break;
         case bitpacking::member_kind::union_internal_tag:
            node.node_name = "union-internal-tag";
            break;
            
         case bitpacking::member_kind::none:
            break;
      }
      
      _apply_x_options_to(node, options);
      
      return node_ptr;
   }
   owned_element report_generator::_generate(const codegen::instructions::transform& instr) {
      auto  node_ptr = std::make_unique<xml_element>();
      auto& node     = *node_ptr;
      
      node.node_name = "transform";
      node.set_attribute("value", _value_path_to_string(instr.to_be_transformed_value));
      node.set_attribute("transformed-type", instr.types.back().name());
      node.set_attribute("transformed-value", _variable_to_string(instr.transformed.descriptors));
      if (instr.types.size() > 1) {
         std::string through;
         for(size_t i = 0; i < instr.types.size() - 1; ++i) {
            if (i > 0)
               through += ' ';
            through += instr.types[i].name();
         }
         node.set_attribute("transform-through", through);
      }
      
      for(auto& child_ptr : instr.instructions) {
         node.append_child(_generate(*child_ptr));
      }
      
      return node_ptr;
   }
   owned_element report_generator::_generate(const codegen::instructions::union_switch& instr) {
      auto  node_ptr = std::make_unique<xml_element>();
      auto& node     = *node_ptr;
      
      node.node_name = "switch";
      node.set_attribute("operand", _value_path_to_string(instr.condition_operand));
      
      for(const auto& pair : instr.cases) {
         auto  child_ptr = std::make_unique<xml_element>();
         auto& child     = *child_ptr;
         child.node_name = "case";
         child.set_attribute("value", _to_string(pair.first));
         node.append_child(std::move(child_ptr));
         
         for(auto& nested : pair.second->instructions) {
            child.append_child(_generate(*nested));
         }
      }
      
      return node_ptr;
   }
   
   owned_element report_generator::_generate(const codegen::instructions::base& instr) {
      if (auto* casted = instr.as<codegen::instructions::array_slice>())
         return _generate(*casted);
      if (auto* casted = instr.as<codegen::instructions::padding>())
         return _generate(*casted);
      if (auto* casted = instr.as<codegen::instructions::single>())
         return _generate(*casted);
      if (auto* casted = instr.as<codegen::instructions::transform>())
         return _generate(*casted);
      if (auto* casted = instr.as<codegen::instructions::union_switch>())
         return _generate(*casted);
      
      assert(false && "unreachable");
   }
   
   owned_element report_generator::_generate_root(const codegen::instructions::container& root) {
      this->_loop_variables.clear();
      this->_transformed_values.clear();
      
      codegen::walk_instruction_nodes(
         [this](const codegen::instructions::base& node) {
            if (auto* casted = node.as<codegen::instructions::array_slice>()) {
               this->_loop_variables.push_back(casted->loop_index.descriptors);
               return;
            }
            if (auto* casted = node.as<codegen::instructions::transform>()) {
               this->_transformed_values.push_back(casted->transformed.descriptors);
               return;
            }
         },
         root
      );
      
      auto  node_ptr = std::make_unique<xml_element>();
      auto& node     = *node_ptr;
      node.node_name = "instructions";
      for(auto& child_ptr : root.instructions) {
         node.append_child(_generate(*child_ptr));
      }
      return node_ptr;
   }
   
   void report_generator::process(const codegen::instructions::container& root) {
      auto& info = this->_sectors.emplace_back();
      
      auto elem_ptr = _generate_root(root);
      info.as_xml->append_child(std::move(elem_ptr));
   }
   void report_generator::process(const codegen::whole_struct_function_dictionary& dict) {
      dict.for_each([this](gw::type::base type, const codegen::whole_struct_function_info& ws_info) {
         auto  elem_ptr = _generate_root(*ws_info.instructions_root->as<codegen::instructions::container>());
         
         auto& info = this->_get_or_create_type_info(type);
         auto& node = *info.as_xml;
         if (auto* prior = node.first_child_by_node_name("instructions")) {
            node.remove_child(*prior);
         }
         node.append_child(std::move(elem_ptr));
      });
   }
   void report_generator::process(const codegen::stats_gatherer& gatherer) {
      {  // Per-sector stats
         size_t end = (std::min)(gatherer.stats_by_sector.size(), this->_sectors.size());
         for(size_t i = 0; i < end; ++i) {
            auto& src  = gatherer.stats_by_sector[i];
            auto& info = this->_sectors[i];
            
            info.stats_data.bitcounts.total_packed = src.total_packed_size;
            
            info.update_stats_xml();
         }
      }
      
      // Per-type stats
      for(const auto& pair : gatherer.stats_by_type) {
         auto         type     = pair.first;
         const auto&  stats    = pair.second;
         
         auto& info = this->_get_or_create_type_info(type);
         
         if (stats.c_align_of > 0) {
            info.c_info.align_of = stats.c_align_of;
         }
         if (stats.c_size_of > 0) {
            info.c_info.size_of = stats.c_align_of;
         }
         info.stats_data.bitcounts.total_packed   = stats.total_sizes.packed;
         info.stats_data.bitcounts.total_unpacked = stats.total_sizes.unpacked;
         
         info.stats_data.counts.total = stats.count_seen.total;
         info.stats_data.counts.by_sector.clear();
         info.stats_data.counts.by_top_level_identifier.clear();
         for(size_t i = 0; i < stats.count_seen.by_sector.size(); ++i) {
            size_t count = stats.count_seen.by_sector[i];
            if (count <= 0)
               continue;
            auto& item = info.stats_data.counts.by_sector.emplace_back();
            item.sector_index = i;
            item.count        = count;
         }
         for(auto& pair : stats.count_seen.by_top_level) {
            auto& item = info.stats_data.counts.by_top_level_identifier.emplace_back();
            item.identifier = pair.first;
            item.count      = pair.second;
         }
         
         info.update_stats_xml();
      }
      
      // Category stats.
      for(auto& pair : gatherer.stats_by_category) {
         const auto& name  = pair.first;
         const auto& stats = pair.second;
         
         auto& info = this->_get_or_create_category_info(name);
         
         info.counts.total = stats.count_seen.total;
         info.bitcounts.total_packed   = stats.total_sizes.packed;
         info.bitcounts.total_unpacked = stats.total_sizes.unpacked;
         
         // Per-sector stats for this category.
         for(size_t i = 0; i < stats.count_seen.by_sector.size(); ++i) {
            auto count = stats.count_seen.by_sector[i];
            if (count == 0)
               continue;
            
            auto& item = info.counts.by_sector.emplace_back();
            item.sector_index = i;
            item.count        = count;
         }
         for(auto& pair : stats.count_seen.by_top_level) {
            auto& item = info.counts.by_top_level_identifier.emplace_back();
            item.identifier = pair.first;
            item.count      = pair.second;
         }
      }
      
      // Done with gatherer.
   }
   
   std::string report_generator::bake() {
      std::string out = "<data>\n";
      {  // global options
         const auto& gs = basic_global_state::get();
         const auto& go = gs.global_options.computed;
         
         out += "   <config>\n";
         {
            auto  node_ptr = std::make_unique<xml_element>();
            auto& node     = *node_ptr;
            node.node_name = "option";
            node.set_attribute("name", "max-sector-count");
            node.set_attribute_i("value", go.sectors.max_count);
            out += node.to_string(2);
            out += '\n';
         }
         if (go.sectors.size_per != std::numeric_limits<size_t>::max()) {
            auto  node_ptr = std::make_unique<xml_element>();
            auto& node     = *node_ptr;
            node.node_name = "option";
            node.set_attribute("name", "max-sector-bytecount");
            node.set_attribute_i("value", go.sectors.size_per);
            out += node.to_string(2);
            out += '\n';
         }
         out += "   </config>\n";
      }
      {  // categories
         auto& list = this->_categories;
         if (!list.empty()) {
            out += "   <categories>\n";
            for(const auto& pair : list) {
               auto node_ptr = pair.second.to_xml();
               node_ptr->node_name = "category";
               node_ptr->set_attribute("name", pair.first);
               
               out += node_ptr->to_string(2);
               out += '\n';
            }
            out += "   </categories>\n";
         }
      }
      {  // seen types
         auto& list = this->_types;
         if (!list.empty()) {
            out += "   <c-types>\n";
            for(auto& info : list) {
               info.update_stats_xml();
               out += info.as_xml->to_string(2);
               out += '\n';
            }
            out += "   </c-types>\n";
         }
      }
      {  // sectors
         auto& list = this->_sectors;
         if (!list.empty()) {
            out += "   <sectors>\n";
            for(auto& info : list) {
               info.update_stats_xml();
               out += info.as_xml->to_string(2);
               out += '\n';
            }
            out += "   </sectors>\n";
         }
      }
      out += "</data>";
      return out;
   }
}