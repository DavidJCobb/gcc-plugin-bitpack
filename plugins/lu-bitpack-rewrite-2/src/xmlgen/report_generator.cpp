#include "xmlgen/report_generator.h"
#include <cassert>
#include <limits>
#include "lu/stringf.h"
#include "bitpacking/data_options.h"
#include "bitpacking/global_options.h"
#include "basic_global_state.h"
#include "codegen/decl_descriptor.h"
#include "codegen/stats_gatherer.h"
#include "codegen/whole_struct_function_dictionary.h"
#include "codegen/whole_struct_function_info.h"
#include "gcc_wrappers/constant/floating_point.h"
#include "gcc_wrappers/constant/integer.h"
#include "gcc_wrappers/constant/string.h"
#include "gcc_wrappers/decl/param.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/type/record.h"
#include "gcc_wrappers/type/untagged_union.h"
namespace gw {
   using namespace gcc_wrappers;
}
namespace typed_options {
   using namespace bitpacking::typed_data_options::computed;
}

#include "codegen/instructions/base.h"
#include "codegen/instructions/array_slice.h"
#include "codegen/instructions/padding.h"
#include "codegen/instructions/single.h"
#include "codegen/instructions/transform.h"
#include "codegen/instructions/union_switch.h"
#include "codegen/instructions/utils/walk.h"

namespace {
   using owned_element = xmlgen::report_generator::owned_element;
}
static std::string _to_string(int v) {
   return lu::stringf("%d", v);
}

namespace xmlgen {
   report_generator::category_info& report_generator::_get_or_create_category_info(std::string_view name) {
      for(auto& info : this->_categories)
         if (info.name == name)
            return info;
      auto& info = this->_categories.emplace_back();
      info.name = name;
      return info;
   }
   report_generator::type_info& report_generator::_get_or_create_type_info(gw::type::base type) {
      for(auto& info : this->_types)
         if (info.stats.type.unwrap() == type.unwrap())
            return info;
      auto& info = this->_types.emplace_back(type);
      return info;
   }
   
   void report_generator::_apply_x_options_to(
      xml_element& node,
      const bitpacking::data_options& options
   ) {
      if (options.is<typed_options::buffer>()) {
         const auto& casted = options.as<typed_options::buffer>();
         node.set_attribute_i("bytecount", casted.bytecount);
      } else if (options.is<typed_options::integral>()) {
         const auto& casted = options.as<typed_options::integral>();
         node.set_attribute_i("bitcount", casted.bitcount);
         node.set_attribute_i("min", casted.min);
         if (casted.max != typed_options::integral::no_maximum) {
            node.set_attribute_i("max", casted.max);
         }
      } else if (options.is<typed_options::pointer>()) {
         ;
      } else if (options.is<typed_options::string>()) {
         const auto& casted = options.as<typed_options::string>();
         node.set_attribute_i("length",    casted.length);
         node.set_attribute_b("nonstring", casted.nonstring);
      } else if (options.is<typed_options::tagged_union>()) {
         const auto& casted = options.as<typed_options::tagged_union>();
         node.set_attribute("tag", casted.tag_identifier);
      } else if (options.is<typed_options::transformed>()) {
         const auto& casted = options.as<typed_options::transformed>();
         if (auto opt = casted.transformed_type)
            node.set_attribute("transformed-type", opt->pretty_print());
         if (auto opt = casted.pre_pack)
            node.set_attribute("pack-function",    opt->name());
         if (auto opt = casted.post_unpack)
            node.set_attribute("unpack-function",  opt->name());
      }
   }
   
   void report_generator::_apply_type_info_to(xml_element& node, gw::type::base type) {
      // Options.
      bitpacking::data_options options;
      options.load(type);
      {
         std::unique_ptr<xml_element> x_opt;
         if (options.is<typed_options::buffer>()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "opaque-buffer-options";
         } else if (options.is<typed_options::integral>()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "integral-options";
         } else if (options.is<typed_options::pointer>()) {
            return;
         } else if (options.is<typed_options::string>()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "string-options";
         } else if (options.is<typed_options::tagged_union>()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "union-options";
         } else if (options.is<typed_options::transformed>()) {
            x_opt = std::make_unique<xml_element>();
            x_opt->node_name = "transform-options";
         }
         if (x_opt != nullptr) {
            _apply_x_options_to(*x_opt, options);
            node.append_child(std::move(x_opt));
         }
      }
   }
   
   std::string report_generator::_loop_variable_to_string(codegen::decl_descriptor_pair pair) const {
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
               return lu::stringf("__loop_var_%u", (int)i);
      }
      return "__unknown_var";
   }
   std::string report_generator::_variable_to_string(codegen::decl_descriptor_pair pair) const {
      for(size_t i = 0; i < this->_transformed_values.size(); ++i)
         if (pair == this->_transformed_values[i])
            return lu::stringf("__transformed_var_%u", (int)i);
      
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
               out += lu::stringf("[%u]", (int)std::get<size_t>(segm.data));
               continue;
            }
            out += '[';
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
      
      codegen::decl_descriptor_pair desc_pair;
      for(auto& segm : instr.value.segments) {
         if (segm.is_array_access())
            continue;
         desc_pair = segm.member_descriptor();
      }
      assert(desc_pair.read != nullptr);
      auto value_type = *desc_pair.read->types.serialized;
      node.set_attribute("type", value_type.pretty_print());
      
      const auto& options = instr.value.bitpacking_options();
      if (options.default_value) {
         auto dv = *options.default_value;
         if (dv.is<gw::constant::string>()) {
            auto sub    = std::make_unique<xml_element>();
            sub->node_name = "default-value-string";
            sub->text_content = dv.as<gw::constant::string>().value();
            node.append_child(std::move(sub));
         } else if (dv.is<gw::constant::integer>()) {
            std::string text;
            {
               auto opt = dv.as<gw::constant::integer>().try_value_signed();
               if (opt.has_value()) {
                  text = lu::stringf("%d", *opt);
               } else {
                  text = "???";
               }
            }
            node.set_attribute("default-value", text);
         } else if (dv.is<gw::constant::floating_point>()) {
            auto text = dv.as<gw::constant::floating_point>().to_string();
            node.set_attribute("default-value", text);
         } else {
            node.set_attribute("default-value", "???");
         }
      }
      if (options.is_omitted) {
         node.node_name = "omitted";
         return node_ptr;
      }
      
      node.node_name = "unknown";
      if (options.is<typed_options::boolean>()) {
         node.node_name = "boolean";
      } else if (options.is<typed_options::buffer>()) {
         node.node_name = "buffer";
      } else if (options.is<typed_options::integral>()) {
         node.node_name = "integer";
      } else if (options.is<typed_options::pointer>()) {
         node.node_name = "pointer";
      } else if (options.is<typed_options::string>()) {
         node.node_name = "string";
      } else if (options.is<typed_options::structure>()) {
         node.node_name = "struct";
      } else if (options.is<typed_options::tagged_union>()) {
         auto& casted = options.as<typed_options::tagged_union>();
         if (casted.is_internal)
            node.node_name = "union-internal-tag";
         else
            node.node_name = "union-external-tag";
      } else if (options.is<typed_options::transformed>()) {
         node.node_name = "transformed";
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
         child.set_attribute_i("value", pair.first);
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
      
      codegen::instructions::utils::walk(
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
      info.instructions = _generate_root(root);
   }
   void report_generator::process(const codegen::whole_struct_function_dictionary& dict) {
      dict.for_each([this](gw::type::base type, const codegen::whole_struct_function_info& ws_info) {
         auto& info = this->_get_or_create_type_info(type);
         info.instructions = _generate_root(*ws_info.instructions_root->as<codegen::instructions::container>());
      });
   }
   void report_generator::process(const codegen::stats_gatherer& gatherer) {
      {  // Per-sector stats
         size_t end = (std::min)(gatherer.sectors.size(), this->_sectors.size());
         for(size_t i = 0; i < end; ++i) {
            auto& src  = gatherer.sectors[i];
            auto& info = this->_sectors[i];
            info.stats = src;
         }
      }
      
      // Per-type stats
      for(const auto& pair : gatherer.types) {
         auto         type  = pair.first;
         const auto&  stats = pair.second;
         
         auto& info = this->_get_or_create_type_info(type);
         info.stats = stats;
      }
      
      // Category stats.
      for(auto& pair : gatherer.categories) {
         const auto& name  = pair.first;
         const auto& stats = pair.second;
         
         auto& info = this->_get_or_create_category_info(name);
         info.stats = stats;
      }
      
      // Done with gatherer.
   }
   
   std::string report_generator::bake() {
      std::string out = "<data>\n";
      {  // global options
         const auto& gs = basic_global_state::get();
         const auto& go = gs.global_options;
         
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
         if (go.sectors.bytes_per != std::numeric_limits<size_t>::max()) {
            auto  node_ptr = std::make_unique<xml_element>();
            auto& node     = *node_ptr;
            node.node_name = "option";
            node.set_attribute("name", "max-sector-bytecount");
            node.set_attribute_i("value", go.sectors.bytes_per);
            out += node.to_string(2);
            out += '\n';
         }
         out += "   </config>\n";
      }
      {  // categories
         auto& list = this->_categories;
         if (!list.empty()) {
            out += "   <categories>\n";
            for(const auto& info : list) {
               auto node_ptr = info.stats.to_xml(info.name);
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
               auto  node_ptr = info.stats.to_xml();
               this->_apply_type_info_to(*node_ptr, info.stats.type);
               
               xml_element* instr = nullptr;
               if (info.instructions) {
                  instr = info.instructions.get();
                  node_ptr->append_child(std::move(info.instructions));
               }
               out += node_ptr->to_string(2);
               out += '\n';
               if (instr) {
                  info.instructions = node_ptr->take_child(*instr);
               }
            }
            out += "   </c-types>\n";
         }
      }
      {  // sectors
         auto& list = this->_sectors;
         if (!list.empty()) {
            out += "   <sectors>\n";
            for(auto& info : list) {
               auto  node_ptr = std::make_unique<xml_element>();
               node_ptr->node_name = "sector";
               node_ptr->append_child(info.stats.to_xml());
               
               xml_element* instr = nullptr;
               if (info.instructions) {
                  instr = info.instructions.get();
                  node_ptr->append_child(std::move(info.instructions));
               }
               out += node_ptr->to_string(2);
               out += '\n';
               if (instr) {
                  info.instructions = node_ptr->take_child(*instr);
               }
            }
            out += "   </sectors>\n";
         }
      }
      out += "</data>";
      return out;
   }
}