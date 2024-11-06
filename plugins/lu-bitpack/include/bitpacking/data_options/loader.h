#pragma once
#include <variant>
#include "bitpacking/data_options/computed.h"
#include "bitpacking/data_options/requested_via_attributes.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/type/base.h"

namespace bitpacking {
   namespace global_options {
      class computed;
   }
}

namespace bitpacking::data_options {
   class loader {
      public:
         enum class result_code {
            success,
            failed,
            field_is_omitted,
         };
      
      public:
         loader(const global_options::computed& g) : global(g) {}
      
         const global_options::computed& global;
         gcc_wrappers::decl::field       field;
         
      protected:
         
         // Mutually exclusive sets of options that need to be coalesced across 
         // heritables and attributes, checked for consistency, and possibly 
         // processed further before eventually being written to the result.
         struct coalescing_options {
            requested_x_options::variant data;
            struct {
               gcc_wrappers::decl::function pre_pack;
               gcc_wrappers::decl::function post_unpack;
            } transforms;
            
            bool as_opaque_buffer = false;
            bool omit = false;
            
            void try_coalesce(tree report_errors_on, const heritable_options&);
         };
         
         void _coalesce_heritable(tree report_errors_on, coalescing_options&, const heritable_options&);
         void _coalesce_attributes(tree report_errors_on, coalescing_options&, const requested_via_attributes&);
         
      public:
         std::optional<computed> result;
      
         result_code go();
   };
}