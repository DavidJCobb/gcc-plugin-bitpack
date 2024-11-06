#pragma once
#include <optional>
#include <variant>
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/attribute.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/function.h"
#include "bitpacking/in_progress_options.h"
#include "bitpacking/member_kind.h"

namespace bitpacking {
   namespace global_options {
      class computed;
   }
   class heritable_options;
}

namespace bitpacking {
   //
   // Bitpacking options for a type or field.
   //
   class data_options {
      public:
         struct buffer_data {
            size_t bytecount = 0;
         };
         struct integral_data { // also boolean, pointer
            size_t    bitcount = 0;
            intmax_t  min = 0;
            uintmax_t max = 0;
         };
         struct string_data {
            size_t length = 0;
            bool   with_terminator = false;
         };
      
      public:
         bool omit_from_bitpacking = false;
         
         member_kind kind = member_kind::none;
         
         std::variant<
            std::monostate,
            buffer_data,
            integral_data,
            string_data
         > data;
         
         struct {
            gcc_wrappers::decl::function pre_pack;
            gcc_wrappers::decl::function post_unpack;
         } transforms;
         
      public:
         void from_field(
            const global_options::computed&,
            gcc_wrappers::decl::field
         );
         
         constexpr bool is_buffer() const noexcept {
            return std::holds_alternative<buffer_data>(this->data);
         }
         constexpr bool is_integral() const noexcept {
            return std::holds_alternative<integral_data>(this->data);
         }
         constexpr bool is_string() const noexcept {
            return std::holds_alternative<string_data>(this->data);
         }
         
         constexpr const buffer_data& buffer_options() const noexcept {
            return std::get<buffer_data>(this->data);
         }
         constexpr const integral_data& integral_options() const noexcept {
            return std::get<integral_data>(this->data);
         }
         constexpr const string_data& string_options() const noexcept {
            return std::get<string_data>(this->data);
         }
         //
         // non-const:
         //
         constexpr buffer_data& buffer_options() noexcept {
            return std::get<buffer_data>(this->data);
         }
         constexpr integral_data& integral_options() noexcept {
            return std::get<integral_data>(this->data);
         }
         constexpr string_data& string_options() noexcept {
            return std::get<string_data>(this->data);
         }
   };
   
   struct data_options_loader {
      public:
         struct option_set {
            bool omit = false;
            
            const heritable_options* inherit = nullptr;
            
            struct {
               gcc_wrappers::decl::function pre_pack;
               gcc_wrappers::decl::function post_unpack;
            } transforms;
            
            bool as_opaque_buffer = false;
            
            in_progress_options::integral integral;
            std::optional<in_progress_options::string> string;
         };
      
      public:
         const global_options::computed& global;
         gcc_wrappers::decl::field       field;
         
      protected:
         // Contextual information for error reporting.
         struct {
            gcc_wrappers::attribute_list attributes;
            std::variant<
               std::monostate,
               gcc_wrappers::type::base,
               gcc_wrappers::decl::field
            > attributes_of
         } context;
         
         // Options loaded from attributes.
         struct {
            option_set type;
            option_set field;
         } seen_options;
         
         std::string _context_name() const;
         location_t  _context_location() const;
         
         // These are defined in the CPP file, since they should never be used 
         // by any outside code.
         //
         template<typename... Args>
         void _report_warning(const char* fmt, Args&&...) const;
         //
         template<typename... Args>
         void _report_error(const char* fmt, Args&&...) const;
         
         void _warn_on_extra_attribute_args(gcc_wrappers::attribute, size_t expected) const;
         
         std::string _pull_heritable_name(gcc_wrappers::attribute_list) const;
         
         bool _can_be_transform_function(gcc_wrappers::type::function) const;
         //
         bool _check_pre_pack_function(
            std::string_view blame,
            gcc_wrappers::decl::function,
            gcc_wrappers::type::function
         ) const; // emits error messages
         //
         bool _check_post_unpack_function(
            std::string_view blame,
            gcc_wrappers::decl::function,
            gcc_wrappers::type::function
         ) const; // emits error messages
         //
         void _check_transform_functions_match(gcc_wrappers::type::function pre_pack, gcc_wrappers::type::function post_unpack);
         
         void _pull_option_set(option_set&);
         
         // Mutually exclusive sets of options that need to be coalesced across 
         // heritables and attributes, checked for consistency, and possibly 
         // processed further before eventually being written to the result.
         struct coalescing_options {
            in_progress_options::variant data;
            
            bool as_opaque_buffer = false;
            bool omit = false;
            
            void try_coalesce(const heritable_options&);
         };
         
         std::string_view _name_of_coalescing_type(const in_progress_options::variant&) const;
         
         void _coalesce_heritable(coalescing_options&, const heritable_options&);
         void _coalesce_attributes(coalescing_options&, const option_set&);
         
      public:
         bool success = true;
         data_options result;
      
         void go();
   };
}