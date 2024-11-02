#pragma once

#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-common.h> // lookup_name; various predefined type nodes
#include <diagnostic.h>
#include <stringpool.h> // get_identifier

#include "gcc_helpers/compare_to_template_param.h"

template<typename FnPtr>
struct bp_func_decl_item {
   bp_func_decl_item() {}
   bp_func_decl_item(const char* name) {
      this->lookup_and_store(name);
   }
   
   const char* name = nullptr;
   tree        decl = NULL_TREE;
   
   void lookup_and_store(const char* name) {
      this->name = name;
      this->decl = lookup_name(get_identifier(name));
   }
   
   bool check() const {
      if (this->decl == NULL_TREE) {
         error("failed to find function %<%s%>", this->name);
         return false;
      }
      if (TREE_CODE(this->decl) != FUNCTION_DECL) {
         error("declaration %<%s%> is not a function", this->name);
         return false;
      }
      if (!gcc_helpers::function_decl_matches_template_param<FnPtr>(this->decl)) {
         error("function %<%s%> does not have the expected signature", this->name);
         return false;
      }
      return true;
   }
};

struct bp_func_decl_set {
   bp_func_decl_item<void(*)(struct lu_BitstreamState*, uint8_t*)>
   bitstream_initialize{"lu_BitstreamInitialize"};
   
   bp_func_decl_item<uint8_t(*)(struct lu_BitstreamState*)>
   bitstream_read_bool{"lu_BitstreamRead_bool"};
   
   bp_func_decl_item<uint8_t(*)(struct lu_BitstreamState*, uint8_t)>
   bitstream_read_u8{"lu_BitstreamRead_u8"};
   
   bp_func_decl_item<uint16_t(*)(struct lu_BitstreamState*, uint8_t)>
   bitstream_read_u16{"lu_BitstreamRead_u16"};
   
   bp_func_decl_item<uint32_t(*)(struct lu_BitstreamState*, uint8_t)>
   bitstream_read_u32{"lu_BitstreamRead_u32"};
   
   bp_func_decl_item<void(*)(struct lu_BitstreamState*, uint8_t*, uint16_t)>
   bitstream_read_string{"lu_BitstreamRead_string_optional_terminator"};
   
   bp_func_decl_item<void(*)(struct lu_BitstreamState*, uint8_t*, uint16_t)>
   bitstream_read_string_wt{"lu_BitstreamRead_string"};
   
   bp_func_decl_item<void(*)(struct lu_BitstreamState*, void*, uint16_t)>
   bitstream_read_buffer{"lu_BitstreamRead_buffer"};
   
   //
   
   inline bool check_all() {
      if (!this->bitstream_initialize.check())
         return false;
      if (!this->bitstream_read_bool.check())
         return false;
      if (!this->bitstream_read_u8.check())
         return false;
      if (!this->bitstream_read_u16.check())
         return false;
      if (!this->bitstream_read_u32.check())
         return false;
      if (!this->bitstream_read_string.check())
         return false;
      if (!this->bitstream_read_string_wt.check())
         return false;
      if (!this->bitstream_read_buffer.check())
         return false;
      return true;
   }
};