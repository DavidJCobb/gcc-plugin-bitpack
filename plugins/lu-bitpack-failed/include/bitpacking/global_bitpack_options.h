#pragma once
#include <cstdint>
#include <limits>
#include <string>
#include <vector>
#include "gcc_helpers/extract_pragma_kv_args.h"

struct global_bitpack_options {
   struct function_identifier_set {
      std::string boolean;
      std::string s8;
      std::string s16;
      std::string s32;
      std::string u8;
      std::string u16;
      std::string u32;
      std::string buffer;
      std::string string_no_term;
      std::string string_with_term;
   };
   
   struct sector_layout_item {
      size_t sector_count   = 1;
      size_t variable_count = std::numeric_limits<size_t>::max();
   };
   
   struct {
      std::string initialize;
      function_identifier_set read;
      function_identifier_set write;
   } functions;
   struct {
      size_t count = 1;
      size_t size  = 1;
      
      // allocate different variables to different sectors
      std::vector<sector_layout_item> layout = {
         .sector_layout_item{}
      };
   } sectors;
   struct {
      std::string bitstream_state;
      std::string boolean = "bool";
      std::string buffer_byte_type;
   } types;
   
   // May throw std::runtime_error with an appropriate error message.
   void load_pragma_args(const gcc_helpers::pragma_kv_set&);
};