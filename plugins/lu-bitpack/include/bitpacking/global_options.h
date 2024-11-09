#pragma once
#include <limits>
#include <optional>
#include <string>
#include "gcc_helpers/extract_pragma_kv_args.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/pointer.h"
#include "gcc_wrappers/type/record.h"

namespace bitpacking {
   namespace global_options {
      struct requested {
         public:
            struct function_set {
               std::string boolean;
               std::string s8;
               std::string s16;
               std::string s32;
               std::string u8;
               std::string u16;
               std::string u32;
               std::string buffer;
               std::string string_ut; // string, potentially unterminated in memory
               std::string string_wt; // string, with terminator in memory
            };
         
         public:
            struct {
               std::string stream_state_init;
               function_set read;
               function_set write;
            } functions;
            struct {
               size_t max_count = 1;
               size_t size_per  = std::numeric_limits<size_t>::max(); // size in bytes per sector
            } sectors;
            struct {
               std::string bitstream_state;
               std::string boolean;     // defaults to built-in `bool`
               std::string buffer_byte;
               std::string string_char; // defaults to built-in `char`
            } types;
            
            void consume_pragma_kv_set(const gcc_helpers::pragma_kv_set&);
      };
      
      struct computed {
         public:
            struct function_set {
               gcc_wrappers::decl::function boolean;
               gcc_wrappers::decl::function s8;
               gcc_wrappers::decl::function s16;
               gcc_wrappers::decl::function s32;
               gcc_wrappers::decl::function u8;
               gcc_wrappers::decl::function u16;
               gcc_wrappers::decl::function u32;
               gcc_wrappers::decl::function buffer;
               gcc_wrappers::decl::function string_ut; // string, potentially unterminated in memory
               gcc_wrappers::decl::function string_wt; // string, with terminator in memory
            };
         
         public:
            struct {
               gcc_wrappers::decl::function stream_state_init;
               function_set read;
               function_set save;
            } functions;
            struct {
               size_t max_count = 1;
               size_t size_per  = std::numeric_limits<size_t>::max(); // size in bytes per sector
            } sectors;
            struct {
               gcc_wrappers::type::record  bitstream_state;
               gcc_wrappers::type::pointer bitstream_state_ptr;
               gcc_wrappers::type::base    boolean;
               gcc_wrappers::type::base    buffer_byte;
               gcc_wrappers::type::base    buffer_byte_ptr;
               gcc_wrappers::type::base    string_char;
            } types;
            
            void resolve(const requested&);
            
            //
            // Helpers:
            //
            
            bool type_is_boolean(const gcc_wrappers::type::base) const;
            bool type_is_string(const gcc_wrappers::type::base) const;
            bool type_is_string_or_array_thereof(const gcc_wrappers::type::base) const;
            
            // For any type that would pass `type_is_string_or_array_thereof`, 
            // finds the underlying "string" type and returns its extent (which 
            // will be its length if it requires a null terminator). For example, 
            // given `char names[5][10]` and `char` as our string character type, 
            // this returns 10.
            //
            // Fails an assertion if the type is not a string or array thereof.
            std::optional<size_t> string_extent_for_type(const gcc_wrappers::type::base) const;
            
            // In general, strips all array ranks to get to the innermost value 
            // type. However, if the type is a to-be-bitpacked string (or array 
            // thereof), returns the string char array type instead.
            gcc_wrappers::type::base innermost_value_type(gcc_wrappers::type::base) const;
      };
   }
}