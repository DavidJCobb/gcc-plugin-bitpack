#pragma once
#include <string>
#include <variant>
#include "lu/strings/zview.h"
#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/attribute.h"
#include "gcc_wrappers/list_node.h"

namespace bitpacking::data_options {
   class processed_bitpack_attributes {
      public:
         enum class invalid_tag {};
         
         template<typename T>
         using specified = std::variant<std::monostate, invalid_tag, T>;
         
      protected:
         tree _node = NULL_TREE; // TYPE or DECL
         
      protected:
         gcc_wrappers::attribute _get_attribute();
         gcc_wrappers::attribute _get_or_create_attribute();
         
         tree _get_map(gcc_wrappers::attribute);
         
         // read-only access
         tree _get_value_node(std::string_view key) const;
         
         tree _get_or_create_map_pair(std::string_view key);
         void _set_item(gcc_wrappers::attribute, lu::strings::zview key, tree value);
         
      public:
         processed_bitpack_attributes(gcc_wrappers::decl::base);
         processed_bitpack_attributes(gcc_wrappers::type::base);
      
         specified<size_t> get_bitcount() const;
         void set_bitcount(specified<size_t>);
         
         specified<intmax_t> get_min() const;
         void set_min(specified<intmax_t>);
         
         specified<intmax_t> get_max() const;
         void set_max(specified<intmax_t>);
         
         specified<std::string> get_inherit() const;
         void set_inherit(specified<std::string>);
         
         specified<std::string> get_transform_pre_pack() const;
         void set_transform_pre_pack(specified<std::string>);
         
         specified<std::string> get_transform_post_unpack() const;
         void set_transform_post_unpack(specified<std::string>);
         
         bool get_omit() const;
         void set_omit(bool);
         
         bool get_as_opaque_buffer() const;
         void set_as_opaque_buffer(bool);
   };
}