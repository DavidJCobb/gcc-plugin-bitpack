#pragma once
#include <string_view>
#include <unordered_map>
#include <gcc-plugin.h>
#include <tree.h>
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/expr/local_block.h"
#include "gcc_wrappers/type.h"
#include "gcc_wrappers/value.h"

#include "bitpacking/stream_function_set.h"

namespace codegen {
   class sector_functions_generator {
      public:
         struct expr_pair {
            gcc_wrappers::expr::base read;
            gcc_wrappers::expr::base save;
         };
      
         struct func_pair {
            func_pair();
            
            gcc_wrappers::decl::function read; // void _(state*);
            gcc_wrappers::decl::function save; // void _(state*);
         };
         
         struct value_pair {
            gcc_wrappers::value for_read;
            gcc_wrappers::value for_save;
            
            target access_member(std::string_view);
            target access_nth(size_t); // if the target is an array
            target access_nth(gw::value read_i, gw::value save_i);
         };
         
      public:
         ~sector_functions_generator();
         
         stream_function_set funcs;
      
         std::unordered_map<tree, func_pair>  whole_struct_functions;
         std::unordered_map<tree, structure*> types_to_structures;
         
         std::vector<func_pair> sector_functions;
         
      protected:
         expr_pair _serialize_whole_struct(
            const      structure&,
            value_pair state_ptr,
            value_pair object
         );
         
         expr_pair _serialize_primitive(
            const      structure_member&,
            value_pair state_ptr,
            value_pair object
         );
         
         expr_pair _serialize_array(
            const      structure_member&,
            value_pair state_ptr,
            value_pair object
            size_t     start,
            size_t     count
         );
         
         expr_pair _serialize_object(
            value_pair state_ptr,
            value_pair object
         );
         
      public:
         func_pair get_or_create_whole_struct_functions(const structure&);
         
      protected:
         struct target;
         
         struct in_progress_func_pair : public func_pair {
            public:
               using owner_type = sector_functions_generator;
            
            public:
               gcc_wrappers::expr::local_block read_root;
               gcc_wrappers::expr::local_block save_root;
               
            public:
               void append(expr_pair);
               void commit();
         };
      
         struct in_progress_sector {
            sector_functions_generator& owner;
            //
            size_t                id = 0;
            size_t                bits_remaining = 0;
            in_progress_func_pair functions;
            
            void next();
         };
         struct target {
            gw::value for_read;
            gw::value for_save;
            
            target access_member(std::string_view);
            target access_nth(size_t); // if the target is an array
            target access_nth(gw::value read_i, gw::value save_i);
         };
         struct target_info {
            const structure*        struct_info = nullptr; // filled if `member_info` is a struct
            const structure_member* member_info = nullptr;
         };
         
         target_info _info_for_target(target&);
         
      public:
         structure* discover_struct(gw::type);
         void run();
   };
}
