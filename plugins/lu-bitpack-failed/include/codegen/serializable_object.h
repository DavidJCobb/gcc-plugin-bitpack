#pragma once
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/type.h"
#include "bitpacking/data_options.h"
#include "bitpacking/serializable_object_type_code.h"
#include "bitpacking/stream_function_set.h"

namespace codegen {
   struct serializable_object {
      public:
         using type_code = bitpacking::serializable_object_type_code;
         
         struct container_data {
            std::vector<std::unique_ptr<serializable_object>> subobjects;
         };
         
         struct subobject_data {
            // Containing object.
            serializable_object* container = nullptr;
            
            // Field decl. Will be empty if the container is an array.
            gcc_wrappers::decl::field decl;
         };
         
      protected:
         static serializable_object* _from_field(
            const stream_function_set& global_options,
            gcc_wrappers::decl::field
         );
      
      public:
         static serializable_object* from_type(
            const stream_function_set& global_options,
            gcc_wrappers::type
         );
         
         type_code          serialization_type = type_code::none;
         gcc_wrappers::type type;
         
         bitpacking::data_options::computed computed_bitpack_options;
         
         std::optional<container_data> container_info;
         std::optional<subobject_data> subobject_info;
         
         constexpr bool is_container() const noexcept {
            switch (serialization_type) {
               case type_code::array:
               case type_code::structure:
                  return true;
            }
            return false;
         }
   };
}