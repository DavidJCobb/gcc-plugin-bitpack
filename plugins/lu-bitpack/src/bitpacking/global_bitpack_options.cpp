#include "bitpacking/global_bitpack_options.h"
#include <stdexcept>

void global_bitpack_options::load_pragma_args(const gcc_helpers::pragma_kv_set& set) {
   auto _pull_size = [&set](const char* key, const char* noun, size_t& dst) {
      auto it = set.find(key);
      if (it == set.end())
         return;
      auto& variant = it->second;
      if (!std::holds_alternative<intmax_t>(variant)) {
         std::string message = "the ";
         message += noun;
         message += " must be specified as an integer literal";
         throw std::runtime_error(message);
      }
      auto value = std::get<intmax_t>(variant);
      if (value < 0) {
         std::string message = "the ";
         message += noun;
         message += " must not be zero or negative";
         throw std::runtime_error(message);
      }
      dst = value;
   };
   
   _pull_size(
      "sector_count",
      "bitpack sector count",
      this->sectors.count
   );
   _pull_size(
      "sector_size",
      "bitpack sector size",
      this->sectors.size
   );
   
   auto _pull_identifier = [&set](std::string key, std::string noun, std::string& dst) {
      auto it = set.find(key);
      if (it == set.end())
         return;
      auto& variant = it->second;
      if (variant.index() != 1) {
         std::string message = "the ";
         message += noun;
         message += " must be specified as an identifier";
         throw std::runtime_error(message);
      }
      dst = std::get<1>(variant);
   };
   
   auto _pull_identifier_set = [&_pull_identifier](
      const char* key_frag,
      const char* noun_frag,
      function_identifier_set& dst
   ) {
      auto _name = [key_frag](const char* a, const char* b) -> std::string {
         return std::string(a) + key_frag + b;
      };
      
      _pull_identifier(
         _name("func_", "_bool"),
         _name("bitpacking ", " function for bool values"),
         dst.boolean
      );
      _pull_identifier(
         _name("func_", "_s8"),
         _name("bitpacking ", " function for int8_t values"),
         dst.s8
      );
      _pull_identifier(
         _name("func_", "_s16"),
         _name("bitpacking ", " function for int16_t values"),
         dst.s16
      );
      _pull_identifier(
         _name("func_", "_s32"),
         _name("bitpacking ", " function for int32t values"),
         dst.s16
      );
      _pull_identifier(
         _name("func_", "_u8"),
         _name("bitpacking ", " function for uint8_t values"),
         dst.u8
      );
      _pull_identifier(
         _name("func_", "_u16"),
         _name("bitpacking ", " function for uint16_t values"),
         dst.u16
      );
      _pull_identifier(
         _name("func_", "_u32"),
         _name("bitpacking ", " function for uint32_t values"),
         dst.u16
      );
      _pull_identifier(
         _name("func_", "_buffer"),
         _name("bitpacking ", " function for arbitrary buffers"),
         dst.buffer
      );
      _pull_identifier(
         _name("func_", "_string"),
         _name("bitpacking ", " function for strings with optional terminators"),
         dst.string_no_term
      );
      _pull_identifier(
         _name("func_", "_string_terminated"),
         _name("bitpacking ", " function for strings with terminators"),
         dst.string_with_term
      );
   };
   //
   _pull_identifier_set("read",  "read",  this->functions.read);
   _pull_identifier_set("write", "write", this->functions.write);
   
   _pull_identifier("func_initialize", "bitstream initialize function", this->functions.initialize);
   
   _pull_identifier("bool_typename", "bool typename", this->types.boolean);
   _pull_identifier("buffer_byte_typename", "buffer byte typename", this->types.buffer_byte_type);
   _pull_identifier("bitstream_state_typename", "bitstream state typename", this->types.bitstream_state);
}