#include "bitpacking/data_options/requested.h"
#include "bitpacking/for_each_influencing_entity.h"
#include "attribute_handlers/helpers/type_transitively_has_attribute.h" // TODO: move this
#include <vector>
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/untagged_union.h"
#include "gcc_wrappers/builtin_types.h"
#include "bitpacking/verify_union_external_tag.h"
#include "bitpacking/verify_union_internal_tag.h"
#include "bitpacking/verify_union_members.h"
#include <diagnostic.h> // warning(), etc.
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace {
   std::string _name_of(tree node) {
      assert(node != NULL_TREE);
      std::string result;
      if (gw::type::base::node_is(node)) {
         result = "type %<";
         result += gw::type::base::from_untyped(node).pretty_print();
         result += "%>";
         return result;
      }
      if (gw::decl::field::node_is(node)) {
         auto decl = gw::decl::field::from_untyped(node);
         
         std::string result = "field %<";
         result += decl.fully_qualified_name();
         result += "%>";
         return result;
      }
      return "<?>";
   }
   location_t _location_of(tree node) {
      gw::decl::base decl;
      if (gw::type::base::node_is(node)) {
         decl = gw::type::base::from_untyped(node).declaration();
      } else if (gw::decl::field::node_is(node)) {
         decl = gw::decl::field::from_untyped(node);
      }
      if (decl.empty())
         return UNKNOWN_LOCATION;
      return decl.source_location();
   }

   template<typename... Args>
   void _report_warning(tree subject, const char* fmt, Args&&... args) {
      std::string message = "unusual bitpacking options for ";
      message += _name_of(subject);
      message += ": ";
      message += fmt;
      warning_at(_location_of(subject), OPT_Wpragmas, message.c_str(), args...);
   }
}

namespace bitpacking::data_options {
   void requested::_load_impl(tree subject, gw::attribute_list attributes) {
      if (!this->has_attr_nonstring) {
         gw::type::base type;
         if (DECL_P(subject)) {
            type.set_from_untyped(TREE_TYPE(subject));
            if (attributes.has_attribute("lu_nonstring") || attributes.has_attribute("nonstring"))
               this->has_attr_nonstring = true;
         } else if (TYPE_P(subject)) {
            type.set_from_untyped(subject);
         }
         if (!this->has_attr_nonstring && !type.empty()) {
            if (attribute_handlers::helpers::type_transitively_has_attribute(type, "lu_nonstring"))
               this->has_attr_nonstring = true;
         }
      }
      
      for(auto attr : attributes) {
         auto key = attr.name();
         
         if (key == "lu bitpack invalid attributes") {
            this->failed = true;
            continue;
         }
         
         //
         // Attribute arguments will have been validated in advance: GCC 
         // verified that the attributes have the right number of args, 
         // and our attribute handlers rejected any obviously malformed 
         // args. We can just access things blindly.
         //
         
         if (key == "lu_bitpack_omit") {
            this->omit = true;
            continue;
         }
         if (key == "lu_bitpack_default_value") {
            this->default_value_node = attr.arguments().front().as_untyped();
            continue;
         }
         if (key == "lu_bitpack_union_member_id") {
            this->union_member_id = attr.arguments().front().as<gw::expr::integer_constant>().value<intmax_t>();
            continue;
         }
         if (key == "lu_bitpack_stat_category") {
            auto str = attr.arguments().front().as<gw::expr::string_constant>();
            this->stat_categories.push_back(str.value());
            continue;
         }
         
         // X-Options: buffer
         if (key == "lu_bitpack_as_opaque_buffer") {
            this->x_options.emplace<requested_x_options::buffer>();
            continue;
         }
         
         // X-Options: integral
         if (key == "lu_bitpack_bitcount") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::integral>(dst_var))
               dst_var.emplace<requested_x_options::integral>();
            auto& dst_data = std::get<requested_x_options::integral>(dst_var);
            
            dst_data.bitcount = attr.arguments().front().as<gw::expr::integer_constant>().value<intmax_t>();
            continue;
         }
         if (key == "lu_bitpack_range") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::integral>(dst_var))
               dst_var.emplace<requested_x_options::integral>();
            auto& dst_data = std::get<requested_x_options::integral>(dst_var);
            
            auto args = attr.arguments();
            dst_data.min = args[0].as<gw::expr::integer_constant>().value<intmax_t>();
            dst_data.max = args[1].as<gw::expr::integer_constant>().value<intmax_t>();
            continue;
         }
         
         // X-Options: string
         if (key == "lu_bitpack_string") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::string>(dst_var))
               dst_var.emplace<requested_x_options::string>();
            auto& dst_data = std::get<requested_x_options::string>(dst_var);
            
            if (this->has_attr_nonstring)
               dst_data.nonstring = true;
            
            continue;
         }
         
         // X-Options: tagged union
         if (key == "lu_bitpack_union_external_tag") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::tagged_union>(dst_var))
               dst_var.emplace<requested_x_options::tagged_union>();
            auto& dst_data = std::get<requested_x_options::tagged_union>(dst_var);
            
            dst_data.tag_identifier = IDENTIFIER_POINTER(attr.arguments().front().as_untyped());
            dst_data.is_external    = true;
            continue;
         }
         if (key == "lu_bitpack_union_internal_tag") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::tagged_union>(dst_var))
               dst_var.emplace<requested_x_options::tagged_union>();
            auto& dst_data = std::get<requested_x_options::tagged_union>(dst_var);
            
            dst_data.tag_identifier = IDENTIFIER_POINTER(attr.arguments().front().as_untyped());
            dst_data.is_internal    = true;
            continue;
         }
         
         // X-Options: transforms
         if (key == "lu_bitpack_transforms") {
            auto& dst_var  = this->x_options;
            if (!std::holds_alternative<requested_x_options::transforms>(dst_var))
               dst_var.emplace<requested_x_options::transforms>();
            auto& dst_data = std::get<requested_x_options::transforms>(dst_var);
            
            auto args   = attr.arguments();
            auto node_a = args[0];
            auto node_b = args[1];
            if (!node_a.empty()) {
               dst_data.pre_pack = node_a.as<gw::decl::function>();
               dst_data.transformed_type = dst_data.pre_pack.function_type().nth_argument_type(1).remove_pointer();
            }
            if (!node_b.empty()) {
               dst_data.post_unpack = node_b.as<gw::decl::function>();
               dst_data.transformed_type = dst_data.pre_pack.function_type().nth_argument_type(1).remove_pointer().remove_const();
            }
            continue;
         }
         
         if (key.starts_with("lu_bitpack_")) {
            _report_warning(subject, "unrecognized bitpacking attribute %<%s%>", key.data());
         }
      }
   }
   void requested::_validate_union(gcc_wrappers::type::base type) {
      gw::type::untagged_union union_type;
      {
         auto here = type;
         while (here.is_array())
            here = here.as_array().value_type();
         if (here.is_union())
            union_type = here.as_union();
      }
      
      auto* casted = std::get_if<requested_x_options::tagged_union>(&this->x_options);
      if (!casted) {
         if (!union_type.empty() && !std::holds_alternative<requested_x_options::buffer>(this->x_options)) {
            //
            // Can't bitpack a union if we don't understand how it's tagged 
            // (and if it's not an opaque buffer).
            //
            this->failed = true;
         }
         return;
      }
      
      if (casted->is_external && casted->is_internal) {
         //
         // Contradictory tag (both internally and externally tagged).
         //
         this->failed = true;
         return;
      }
      
      if (union_type.empty()) {
         //
         // Why are tagged-union options being applied to a non-union?
         //
         this->failed = true;
         return;
      }
      //
      // Union types have already been validated, but we have no way of marking 
      // types with an error sentinel, so we have to do extra error checking 
      // here.
      //
      if (!verify_union_members(union_type, true)) {
         this->failed = true;
         return;
      }
      if (casted->is_internal) {
         if (!verify_union_internal_tag(union_type, casted->tag_identifier, true)) {
            this->failed = true;
            return;
         }
      }
   }
}