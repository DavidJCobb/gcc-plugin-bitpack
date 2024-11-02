#pragma once
#include <cstdint>
#include <limits>
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/expr/base.h"
#include "gcc_wrappers/type.h"

namespace bitpacking {
   class stream_function_set {
      protected:
         using func_decl = gw::decl::function;
         
      public:
         struct expression_options {
            struct {
               size_t bytecount = std::numeric_limits<size_t>::max();
            } buffer;
            struct {
               size_t   bitcount = std::numeric_limits<size_t>::max();
               intmax_t min = 0;
            } integral;
            struct {
               size_t max_length = std::numeric_limits<size_t>::max();
               bool   needs_terminator = true;
            } string;
         };
      
      public:
         stream_function_set();
      
         struct by_operation {
            func_decl boolean   = nullptr;
            func_decl s8        = nullptr;
            func_decl s16       = nullptr;
            func_decl s32       = nullptr;
            func_decl u8        = nullptr;
            func_decl u16       = nullptr;
            func_decl u32       = nullptr;
            func_decl buffer    = nullptr;
            func_decl string_ut = nullptr; // no terminator byte in memory
            func_decl string_wt = nullptr; // must have terminator byte in memory
         };
         
         func_decl    stream_state_init = nullptr;
         by_operation read;
         by_operation save;
         struct {
            gcc_wrappers::type boolean;      // if not specified, defaults to built-in `bool`
            gcc_wrappers::type buffer_byte;
            gcc_wrappers::type stream_state; // NOT a pointer type
            gcc_wrappers::type string_char;  // if not specified, defaults to built-in `char`
         } types;
         
         // throws std::runtime_error with text, if any functions invalid
         void check_all() const;
         
         // Create a bitstream-read-and-assign expression for an integral, string, or 
         // buffer value, e.g.
         //
         //    a = lu_BitstreamRead_T(state, bitcount);
         //    lu_BitstreamRead_string(state, b, b_length);
         //
         // The only allowed items to serialize this way are integrals, pointers, strings, 
         // and buffers. Arrays are assumed to be strings (we assert that the element type 
         // matches our chosen char type). Pointers are assumed to be buffers (we cast to 
         // a pointer of the buffer-byte type).
         gcc_wrappers::expr::base make_read_expression_for(
            gcc_wrappers::value state_pointer,
            gcc_wrappers::value item,
            const expression_options&
         ) const;
         
         // Create a bitstream-write expression for an integral, string, or buffer value, 
         // e.g.
         //
         //    lu_BitstreamWrite_T(state, a, bitcount);
         //    lu_BitstreamWrite_string(state, b, b_length);
         //
         // The only allowed items to serialize this way are integrals, pointers, strings, 
         // and buffers. Arrays are assumed to be strings (we assert that the element type 
         // matches our chosen char type). Pointers are assumed to be buffers (we cast to 
         // a pointer of the buffer-byte type).
         gcc_wrappers::expr::base make_save_expression_for(
            gcc_wrappers::value state_pointer,
            gcc_wrappers::value item,
            const expression_options&
         ) const;
   };
}