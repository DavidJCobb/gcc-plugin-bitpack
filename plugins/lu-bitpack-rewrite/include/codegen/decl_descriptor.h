#pragma once
#include <limits>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>
#include "lu/singleton.h"
#include "bitpacking/data_options/computed.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/param.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/type/base.h"

namespace codegen {
   class decl_descriptor {
      public:
         // Used in the `array.extents` list to indicate a variable-length array.
         static constexpr const size_t vla_extent = std::numeric_limits<size_t>::max();
      
      protected:
         // assumes `types.basic_type` is already set
         // assumes `options` is already set
         void _compute_types();
      
      public:
         decl_descriptor(gcc_wrappers::decl::field);
         decl_descriptor(gcc_wrappers::decl::param);
         decl_descriptor(gcc_wrappers::decl::variable);
         
      public:
         gcc_wrappers::decl::base           decl;
         bitpacking::data_options::computed options;
         struct {
            gcc_wrappers::type::base basic_type; // of the decl that was passed in
            gcc_wrappers::type::base innermost;  // relevant for arrays
            gcc_wrappers::type::base serialized; // e.g. array for strings; also accounts for transforms
            
            // In transformation order: if A -> B and B -> C, this is [A, B, C].
            std::vector<gcc_wrappers::type::base> transformations;
         } types;
         struct {
            std::vector<size_t> extents; // for convenience: array extents leading to `serialized`
         } array;
         
         // Gets the members of the `serialized` type, as decl descriptors.
         // Asserts that said type is a struct or union. Members may be 
         // omitted from bitpacking; the caller must check.
         std::vector<const decl_descriptor*> members_of_serialized() const;
         
         size_t serialized_type_size_in_bits() const;
         
         bool is_or_contains_defaulted() const;
   };
   
   class decl_dictionary;
   class decl_dictionary : public lu::singleton<decl_dictionary> {
      protected:
         std::unordered_map<gcc_wrappers::decl::base, std::unique_ptr<decl_descriptor>> _data;
         
         // For transformations defined on types. If a type is known not to be 
         // transformed, then it's in here as an empty type so that if we see 
         // it again, we can check it faster. (We store optionals so that we 
         // can distinguish "never checked for" from "not transformed.")
         //
         // key, value = original, transformed
         std::unordered_map<gcc_wrappers::type::base, std::optional<gcc_wrappers::type::base>> _transformations;
      
      public:
         const decl_descriptor& get_or_create_descriptor(gcc_wrappers::decl::field);
         const decl_descriptor& get_or_create_descriptor(gcc_wrappers::decl::param);
         const decl_descriptor& get_or_create_descriptor(gcc_wrappers::decl::variable);
         
         gcc_wrappers::type::base type_transforms_into(gcc_wrappers::type::base);
   };
}