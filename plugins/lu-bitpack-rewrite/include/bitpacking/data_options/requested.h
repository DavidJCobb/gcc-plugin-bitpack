#pragma once
#include <optional>
#include <variant>
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
         
         std::variant<
            std::monostate,
            requested_x_options::buffer,
            requested_x_options::integral,
            requested_x_options::string,
            requested_x_options::transforms
         > x_options;
         
         bool failed = false;
         
      protected:
         void _load_impl(tree, gcc_wrappers::attribute_list);
   
      public:
         bool load(gcc_wrappers::type::base);
         
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
            return this->failed;
         }
   };
}