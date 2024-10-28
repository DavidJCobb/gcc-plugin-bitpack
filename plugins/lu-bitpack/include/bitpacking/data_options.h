#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/list_node.h"
#include "gcc_wrappers/type.h"
#include "bitpacking/serializable_object_type_code.h"

namespace bitpacking {
   namespace data_options {
      struct transform_funcs {
         std::string_view pre_pack;
         std::string_view post_unpack;
      };
      
      struct computed_buffer_options {
         size_t bytecount = 0;
      };
      struct computed_integral_options {
         size_t   bitcount = 0;
         intmax_t min      = 0;
      };
      struct computed_string_options {
         size_t length = 0;
         bool   null_terminated = false;
      };
      
      struct requested {
         bool        do_not_serialize = false;
         bool        as_opaque_buffer = false;
         bool        as_string        = false;
         std::string inherit_from;
         
         transform_funcs transform;
         struct {
            std::string_view pre_pack;
            std::string_view post_unpack;
         } funcs;
         struct {
            std::optional<uint8_t>  bitcount;
            std::optional<int64_t>  min;
            std::optional<uint64_t> max;
         } integral;
         struct {
            std::optional<size_t> length;
            std::optional<bool>   null_terminated;
         } string;
         
         void from_decl(gw:decl::field);
         
         bool requests_integral() const;
         bool requests_string() const;
      };
      
      struct computed {
         protected:
            void _from_type(gcc_wrappers::type, const requested&);
         
         public:
            computed() {}
            computed(serializable_object_type_code, gcc_wrappers::type, const requested&);
            computed(serializable_object_type_code, gcc_wrappers::decl::field, const requested&);
         
         public:
            transform_funcs transform;
            std::variant<
               std::monostate,
               computed_buffer_options,
               computed_integral_options,
               computed_string_options
            > packing;
      };
   }
}