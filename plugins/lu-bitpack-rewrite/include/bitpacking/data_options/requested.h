#pragma once
#include <optional>
#include <variant>
#include <vector>
#include "bitpacking/data_options/x_options.h"
#include "bitpacking/for_each_influencing_entity.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/decl/param.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/attribute.h"

namespace bitpacking::data_options {
   struct requested {
      public:
         bool omit  = false;
         bool track = false;
         
         tree default_value_node = NULL_TREE;
         bool has_attr_nonstring = false;
         
         std::optional<intmax_t> union_member_id;
         
         std::variant<
            std::monostate,
            requested_x_options::buffer,
            requested_x_options::integral,
            requested_x_options::string,
            requested_x_options::tagged_union,
            requested_x_options::transforms
         > x_options;
         
         std::vector<std::string> stat_categories;
         
         bool failed = false;
         
      protected:
         void _load_impl(tree, gcc_wrappers::attribute_list);
         void _validate_union(gcc_wrappers::type::base);
   
      public:
         template<typename Entity> requires (
            std::is_same_v<Entity, gcc_wrappers::type::base>  ||
            std::is_same_v<Entity, gcc_wrappers::decl::field> ||
            std::is_same_v<Entity, gcc_wrappers::decl::variable>
         )
         bool load(Entity entity) {
            for_each_influencing_entity(entity, [this](tree node) {
               if (TYPE_P(node)) {
                  _load_impl(node, gcc_wrappers::type::base::from_untyped(node).attributes());
               } else {
                  _load_impl(node, gcc_wrappers::decl::base::from_untyped(node).attributes());
               }
            });
            if constexpr (std::is_same_v<Entity, gcc_wrappers::type::base>) {
               this->_validate_union(entity);
            } else {
               this->_validate_union(entity.value_type());
            }
            return this->failed;
         }
   };
}