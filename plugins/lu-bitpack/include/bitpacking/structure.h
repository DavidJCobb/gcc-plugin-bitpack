#pragma once
#include <vector>
#include <gcc-plugin.h>
#include <tree.h>
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/type.h"

namespace codegen {
   enum class member_serialization_type {
      skip,
      boolean,
      buffer,
      integer,
      pointer,
      string,
      substructure,
   };
   
   struct structure_member {
      structure_member() {}
      structure_member(gcc_wrappers::decl::field);
      
      gcc_wrappers::decl::field decl;
      gcc_wrappers::type        type;
      gcc_wrappers::type        value_type; // differs if an array or string
      member_serialization_type serialization_type = member_serialization_type::skip;
      struct {
         BitpackOptions specified;
         struct {
            struct {
               size_t bytecount = 0;
            } buffer;
            struct {
               size_t bitcount = 0;
            } integral;
            struct {
               size_t length;
               bool null_terminated = false;
            } string;
            struct {
               size_t bitcount = 0;
            } substructure;
         } computed;
      } options;
      
      void recompute_options();
      
      size_t compute_single_element_packed_bitcount() const;
      size_t compute_single_element_unpacked_bytecount() const;
      
      size_t compute_total_packed_bitcount() const;
      size_t compute_total_unpacked_bitcount() const;
   };

   struct structure {
      structure() {}
      structure(gcc_wrappers::type);
      
      gcc_wrappers::type type;
      std::vector<structure_member> members;
      
      size_t compute_packed_bitcount() const;
      size_t compute_unpacked_bytecount() const;
   };
}