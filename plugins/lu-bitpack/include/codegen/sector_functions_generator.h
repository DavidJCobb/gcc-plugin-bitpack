#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "gcc_wrappers/type.h"
#include "codegen/expr_pair.h"
#include "codegen/func_pair.h"
#include "codegen/value_pair.h"
#include "codegen/in_progress_func_pair.h"
#include "codegen/serialization_value.h"
#include "codegen/descriptors.h"

namespace codegen {
   class sector_functions_generator {
      public:
         sector_functions_generator(bitpacking::global_options::computed&);
         ~sector_functions_generator();
         
      public:
         struct struct_info {
            std::unique_ptr<struct_descriptor> descriptor;
            func_pair whole_struct_functions;
         };
         
         using identifier_group = std::vector<std::string>;
         
      public:
         // Inputs:
         bitpacking::global_options::computed& global_options;
         std::vector<identifier_group> identifiers_to_serialize;
         
         // Outputs:
         std::vector<func_pair> sector_functions;
         func_pair top_level_functions; // void read(const buffer_byte_type* src, int sector_id), etc.
         
      public:
         func_pair get_or_create_whole_struct_functions(const struct_descriptor&);
         
         struct_info& info_for_struct(gcc_wrappers::type);
         
      protected:
         std::unordered_map<gcc_wrappers::type, struct_info> _struct_info;
         
         const struct_descriptor* _descriptor_for_struct(serialization_value&);
         
         expr_pair _serialize_whole_struct(
            value_pair state_ptr,
            serialization_value&
         );
         expr_pair _serialize_array_slice(
            value_pair state_ptr,
            serialization_value&,
            size_t start,
            size_t count
         );
         expr_pair _serialize_primitive(
            value_pair state_ptr,
            serialization_value&
         );
         
         expr_pair _serialize(
            value_pair state_ptr,
            serialization_value&
         );
         
         struct in_progress_sector {
            sector_functions_generator& owner;
            gcc_wrappers::type function_type;
            //
            size_t                id = 0;
            size_t                bits_remaining = 0;
            in_progress_func_pair functions;
            
            // The per-sector read and save functions both use the signature:
            // void f(struct lu_BitstreamState*)
            
            bool empty() const;
            bool has_functions() const;
            void make_functions();
            void next();
         };
         
         void _serialize_value_to_sector(in_progress_sector&, serialization_value&);
         void _make_top_level_functions();
         
      public:
         void run();
   };
}