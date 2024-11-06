#pragma once
#include <optional>
#include <variant>
#include "bitpacking/data_options/x_options.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/attribute.h"

namespace bitpacking {
   class heritable_options;
}

namespace bitpacking::data_options {
   struct requested_via_attributes {
      public:
         bool omit = false;
         
         const heritable_options* inherit = nullptr;
         
         struct {
            gcc_wrappers::decl::function pre_pack;
            gcc_wrappers::decl::function post_unpack;
         } transforms;
         
         bool as_opaque_buffer = false;
         struct {
            requested_x_options::integral integral;
            std::optional<requested_x_options::string> string;
         } x_options;
         
      protected:
         void _load_bitcount(tree report_errors_on, gcc_wrappers::attribute);
         void _load_transforms(tree report_errors_on, gcc_wrappers::attribute);
         void _load_range(tree report_errors_on, gcc_wrappers::attribute);
         void _load_string(tree report_errors_on, gcc_wrappers::attribute);
         
         void _load_impl(tree, gcc_wrappers::attribute_list);
         
      public:
         // Returns false if we emit any errors while trying to load.
         bool load(gcc_wrappers::decl::field);
         bool load(gcc_wrappers::type::base);
   };
}