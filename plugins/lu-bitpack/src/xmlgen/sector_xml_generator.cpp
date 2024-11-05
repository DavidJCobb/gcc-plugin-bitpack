#include "xmlgen/sector_xml_generator.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/decl/variable.h"
#include "bitpacking/heritable_options.h"
#include <charconv> // std::to_chars
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}
#include <diagnostic.h> // debug

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
   std::unique_ptr<xml_element> sector_xml_generator::_make_whole_data_member(const codegen::member_descriptor& member) {
      auto ptr = std::make_unique<xml_element>();
      {
         auto name = member.decl.name();
         if (!name.empty())
            ptr->set_attribute("name", name);
      }
      {
         auto vt = member.value_type;
         ptr->set_attribute("c-type", member.value_type.pretty_print());
         if (vt.is_record() && !vt.declaration().empty()) {
            ptr->set_attribute("c-type-defined-via-typedef", "true");
         }
      }
      switch (member.kind) {
         case bitpacking::member_kind::boolean:
            ptr->node_name = "boolean";
            break;
         case bitpacking::member_kind::buffer:
            ptr->node_name = "opaque-buffer";
            {
               auto& options = member.bitpacking_options.buffer_options();
               ptr->set_attribute("bytecount", _to_chars(options.bytecount));
            }
            break;
         case bitpacking::member_kind::integer:
            ptr->node_name = "integer";
            {
               auto& options = member.bitpacking_options.integral_options();
               ptr->set_attribute("bitcount", _to_chars(options.bitcount));
               ptr->set_attribute("min",      _to_chars(options.min));
            }
            break;
         case bitpacking::member_kind::pointer:
            ptr->node_name = "pointer";
            {
               auto& options = member.bitpacking_options.integral_options();
               ptr->set_attribute("bitcount", _to_chars(options.bitcount));
            }
            break;
         case bitpacking::member_kind::string:
            ptr->node_name = "string";
            {
               auto& options = member.bitpacking_options.string_options();
               ptr->set_attribute("length", _to_chars(options.length));
               ptr->set_attribute("with_terminator", options.with_terminator ? "true" : "false");
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
      for(auto extent : member.array_extents) {
         auto rank_elem = std::make_unique<xml_element>();
         rank_elem->node_name = "array-rank";
         rank_elem->set_attribute("extent", _to_chars(extent));
         ptr->append_child(std::move(rank_elem));
      }
      
      return ptr;
   }
   
   void sector_xml_generator::_make_heritable_xml(std::string_view name) {
      const auto* options = heritable_options_stockpile::get_fast().options_by_name(name);
      assert(options != nullptr);
      
      auto ptr = std::make_unique<xml_element>();
      ptr->node_name = "heritable";
      ptr->set_attribute("name", name);
      
      if (auto* casted = std::get_if<heritable_options::integral_data>(&options->data)) {
         ptr->set_attribute("type", "integer");
         if (auto& opt = casted->bitcount; opt.has_value())
            ptr->set_attribute("bitcount", _to_chars(*opt));
         if (auto& opt = casted->min; opt.has_value())
            ptr->set_attribute("min", _to_chars(*opt));
         if (auto& opt = casted->max; opt.has_value())
            ptr->set_attribute("max", _to_chars(*opt));
      } else if (auto* casted = std::get_if<heritable_options::string_data>(&options->data)) {
         ptr->set_attribute("type", "string");
         if (auto& opt = casted->length; opt.has_value())
            ptr->set_attribute("length", _to_chars(*opt));
         if (auto& opt = casted->with_terminator; opt.has_value())
            ptr->set_attribute("with-terminator", *opt ? "true" : "false");
      }
      
      if (auto& v = options->transforms.pre_pack; !v.empty()) {
         ptr->set_attribute("pre-pack-transform", v);
      }
      if (auto& v = options->transforms.post_unpack; !v.empty()) {
         ptr->set_attribute("post-unpack-transform", v);
      }
      
      this->heritables[std::string(name)] = std::move(ptr);
   }
   
   void sector_xml_generator::_on_struct_seen(gcc_wrappers::type::record type) {
      auto& data = this->whole_struct_xml[type];
      if (data.descriptor != nullptr) {
         return;
      }
      
      data.descriptor = std::make_unique<codegen::struct_descriptor>(this->global_options, type);
      
      auto* root = (data.root = std::make_unique<xml_element>()).get();
      auto& desc = *data.descriptor.get();
      
      root->node_name = "struct-type";
      root->set_attribute("name", type.name());
      root->set_attribute("c-type", type.pretty_print());
      
      for(auto& m_descriptor : desc.members) {
         root->append_child(std::move(_make_whole_data_member(m_descriptor)));
      }
   }
   void sector_xml_generator::_on_struct_seen(const serialization_value& value) {
      _on_struct_seen(value.as_member().type().as_record());
   }
   const codegen::struct_descriptor* sector_xml_generator::_descriptor_for_struct(const serialization_value& value) {
      assert(value.is_struct());
      if (value.is_top_level_struct()) {
         return &value.as_top_level_struct();
      }
      return this->whole_struct_xml[value.as_member().type().as_record()].descriptor.get();
   }
   
   std::unique_ptr<xml_element> sector_xml_generator::_serialize_array_slice(
      const serialization_value& value,
      size_t start,
      size_t count
   ) {
      // e.g.
      // <value path="sStructA.elements[0:5]" /> <!-- indices in the range [0, 5) -->
      // <value path="sStructA.elements[0]" />
      auto path = value.access_nth(start, count).path;
      
      auto ptr = std::make_unique<xml_element>();
      ptr->node_name = "value";
      ptr->set_attribute("path", path);
      return ptr;
   }
   std::unique_ptr<xml_element> sector_xml_generator::_serialize(const serialization_value& value) {
      auto ptr = std::make_unique<xml_element>();
      ptr->node_name = "value";
      ptr->set_attribute("path", value.path);
      return ptr;
   }

   void sector_xml_generator::_serialize_value_to_sector(
      in_progress_sector&        sector,
      const serialization_value& object
   ) {
      if (object.is_top_level_struct()) {
         this->_on_struct_seen(object.as_top_level_struct().type);
      } else if (object.is_member()) {
         auto type = object.as_member().type();
         if (type.is_record())
            this->_on_struct_seen(type.as_record());
      }
      
      const size_t bitcount = object.bitcount();
      if (bitcount <= sector.bits_remaining) {
         sector.root->append_child(this->_serialize(object));
         sector.bits_remaining -= bitcount;
         return;
      }
      
      //
      // The object doesn't fit. We need to serialize it in fragments, 
      // splitting it across a sector boundary.
      //
      
      //
      // Handle top-level and nested struct members:
      //
      if (object.is_struct()) {
         const auto* info = _descriptor_for_struct(object);
         assert(info != nullptr);
         for(const auto& m : info->members) {
            auto v = object.access_member(m);
            this->_serialize_value_to_sector(sector, v);
         }
         return;
      }
      
      //
      // Handle arrays:
      //
      if (object.is_array()) {
         const auto& info = object.as_member();
         
         size_t elem_size = info.element_size_in_bits();
         size_t i         = 0;
         size_t extent    = info.array_extent();
         while (i < extent) {
            //
            // Generate a loop to serialize as many elements as can fit.
            //
            size_t can_fit = sector.bits_remaining / elem_size;
            if (can_fit > 1) {
               if (i + can_fit > extent) {
                  auto excess = i + can_fit - extent;
                  can_fit -= excess;
               }
               sector.root->append_child(
                  this->_serialize_array_slice(object, i, can_fit)
               );
               {
                  size_t consumed = can_fit * elem_size;
                  assert(consumed <= sector.bits_remaining);
                  sector.bits_remaining -= consumed;
               }
               i += can_fit;
               if (i >= extent)
                  break;
            }
            //
            // Split the element that won't fit across a sector boundary.
            //
            this->_serialize_value_to_sector(sector, object.access_nth(i));
            ++i;
            //
            // Repeat this process until the whole array makes it in.
            //
         }
         return;
      }
      
      //
      // Handle indivisible values. We need to advance to the next sector to fit 
      // them.
      // 
      sector.next();
      //
      // Retry in the new sector.
      //
      this->_serialize_value_to_sector(sector, object);
   }
   void sector_xml_generator::run() {
      in_progress_sector sector(*this);
      sector.root = std::make_unique<xml_element>();
      sector.root->node_name = "sector";
      sector.root->set_attribute("id", "0");
      sector.id = 0;
      sector.bits_remaining = this->global_options.sectors.size_per * 8;
      
      for(size_t i = 0; i < this->identifiers_to_serialize.size(); ++i) {
         auto& list = identifiers_to_serialize[i];
         if (i > 0 && !sector.empty()) {
            sector.next();
         }
         
         for(auto& name : list) {
            auto node = lookup_name(get_identifier(name.c_str()));
            assert(node != NULL_TREE);
            assert(TREE_CODE(node) == VAR_DECL);
            
            auto decl = gw::decl::variable::from_untyped(node);
            auto type = decl.value_type();
            assert(type.is_record());
            this->_on_struct_seen(type.as_record());
            
            const auto* descriptor = this->whole_struct_xml[type.as_record()].descriptor.get();
            
            serialization_value value;
            value.descriptor = descriptor;
            value.path       = name;
            
            this->_serialize_value_to_sector(sector, value);
         }
      }
      this->per_sector_xml.push_back(std::move(sector.root));
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
         auto& list = this->whole_struct_xml;
         if (!list.empty()) {
            out += "   <types>\n";
            for(const auto& pair : list) {
               out += pair.second.root->to_string(2);
               out += '\n';
            }
            out += "   </types>\n";
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
   
   //
   // sector_xml_generator::in_progress_sector
   //
   
   bool sector_xml_generator::in_progress_sector::empty() const {
      return this->bits_remaining == this->owner.global_options.sectors.size_per * 8;
   }
   void sector_xml_generator::in_progress_sector::next() {
      this->owner.per_sector_xml.push_back(std::move(this->root));
      ++this->id;
      this->bits_remaining = this->owner.global_options.sectors.size_per * 8;
      assert(this->id < this->owner.global_options.sectors.max_count);
      //
      this->root = std::make_unique<xml_element>();
      this->root->node_name = "sector";
      this->root->set_attribute("id", _to_chars(this->id));
   }
}