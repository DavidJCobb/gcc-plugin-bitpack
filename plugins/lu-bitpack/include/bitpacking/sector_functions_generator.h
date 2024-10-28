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

#include "bp_func_decl_set.h"

namespace codegen {
   class sector_functions_generator {
      public:
         struct func_pair {
            func_pair();
            
            gcc_wrappers::decl::function read; // void _(state*);
            gcc_wrappers::decl::function save; // void _(state*);
         };
         
      public:
         ~sector_functions_generator();
         
         bp_func_decl_set funcs;
      
         std::unordered_map<tree, func_pair*> whole_struct_functions;
         std::unordered_map<tree, structure*> types_to_structures;
         
         std::vector<func_pair> sector_functions;
         
      protected:
         struct target;
         
         struct in_progress_func_pair : public func_pair {
            public:
               using owner_type = sector_functions_generator;
            
            public:
               gcc_wrappers::expr::local_block read_root;
               gcc_wrappers::expr::local_block save_root;
               
            protected:
               gcc_wrappers::expr::base _read_expr_non_struct(
                  owner_type&,
                  gcc_wrappers::value,
                  const structure_member&
               );
            
            public:
               void serialize_array_slice(
                  owner_type&,
                  const structure_member&,
                  target&,
                  size_t at,
                  size_t count
               );
               void serialize_entire(
                  owner_type&,
                  const structure&,
                  target&
               );
               void serialize_entire(
                  owner_type&,
                  const structure_member&,
                  target&
               );
               
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
