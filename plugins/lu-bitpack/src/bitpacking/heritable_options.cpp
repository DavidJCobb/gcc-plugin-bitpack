#include "bitpacking/heritable_options.h"
#include "lu/strings/printf_string.h"
#include "gcc_helpers/extract_pragma_kv_args.h"
#include <tree.h>
#include <diagnostic.h>

namespace {
   template<typename... Args>
   void _report_error(location_t, const char* fmt, Args&&... args) {
      std::string str = "#pragma lu-bitpack heritable: ";
      str += lu::strings::printf_string(fmt, args...);
      error_at(loc, str.c_str());
   }
   
   template<typename... Args>
   void _report_warning(location_t loc, const char* fmt, Args&&... args) {
      std::string str = "#pragma lu-bitpack heritable: ";
      str += lu::strings::printf_string(fmt, args...);
      warning_at(loc, 1, str.c_str());
   }
   
   struct _kv_reader {
      public:
         cpp_reader* reader = nullptr;
         
         std::string heritable_name;
         
      protected:
         std::string_view current_key;
         
         bool _consume_kv_name(location_t loc, int token) {
            if (token == CPP_NAME) {
               this->current_key = IDENTIFIER_POINTER(data);
            } else if (token == CPP_STRING) {
               this->current_key = TREE_STRING_POINTER(data);
            } else {
               _report_error(loc, "key expected for key/value pair in options %%<%s%%>", this->heritable_name.c_str());
               return false;
            }
            return true;
         }
      
         bool _consume_kv_separator() {
            location_t loc;
            tree       data;
            if (pragma_lex(&data, &loc) != CPP_EQ) {
               _report_error(
                  loc,
                  "key/value pair separator (%%<=%%>) expected after key %%<%s%%> in options %%<%s%%>",
                  this->current_key.data(),
                  this->heritable_name.c_str()
               );
               return false;
            }
            return true;
         }
         
         std::optional<int> _consume_kv_value_number() {
            location_t loc;
            tree       data;
            if (pragma_lex(&data, &loc) != CPP_NUMBER) {
               _report_error(
                  loc,
                  "integer literal expected as value for key %%<%s%%> in options %%<%s%%>",
                  this->current_key.data(),
                  this->heritable_name.c_str()
               );
               return {};
            }
            if (TREE_CODE(data) != INTEGER_CST) {
               _report_error(
                  loc,
                  "invalid integer constant supplied for key %%<%s%%> in options %%<%s%%>",
                  this->current_key.data(),
                  this->heritable_name.c_str()
               );
               return {};
            }
            return (int) TREE_INT_CST_LOW(data);
         }
         
         void _report_unrecognized_key(location_t loc, const char* heritable_type) {
            _report_warning(
               loc,
               "unrecognized key %%<%s%%> in %s options %%<%s%%>",
               heritable_type,
               this->current_key.data(),
               this->heritable_name.c_str()
            );
         }
         
      public:
         bool load(bitpacking::heritable_options::integral_data& dst) {
            assert(this->reader != nullptr);
            
            location_t loc;
            tree       data;
            auto token = pragma_lex(&data, &loc);
            if (token == CPP_CLOSE_PAREN)
               return true;
            
            do {
               if (!_consume_kv_name(loc, token))
                  return false;
               if (!_consume_kv_separator())
                  return false;
               
               int v;
               if (!_consume_kv_value_number())
                  return false;
               
               if (key == "bitcount") {
                  if (v <= 0) {
                     _report_error(loc, "you cannot use a zero or negative bitcount (%%<%s=%d%%> in options %%<%s%%>", this->current_key.data(), v, this->heritable_name.c_str());
                     return false;
                  }
                  dst.bitcount = v;
               } else if (key == "min") {
                  dst.min = v;
               } else if (key == "max") {
                  dst.max = v;
               } else {
                  _report_unrecognized_key(loc, "integer");
               }
               
               token = pragma_lex(&data, &loc);
               if (token == CPP_CLOSE_PAREN)
                  break;
               if (token == CPP_COMMA) {
                  token = pragma_lex(&data, &loc);
                  continue;
               }
               
               _report_error(loc, "expected %%<,%%> or %%<)%%> while parsing options %%<%s%%>", this->heritable_name.c_str());
               return false;
            } while (true);
            
            return true;
         }
         
         bool load(bitpacking::heritable_options::string_data& dst) {
            assert(this->reader != nullptr);
            
            location_t loc;
            tree       data;
            auto token = pragma_lex(&data, &loc);
            if (token == CPP_CLOSE_PAREN)
               return true;
            
            do {
               if (!_consume_kv_name(loc, token))
                  return false;
               
               if (key == "length") {
                  if (!_consume_kv_separator())
                     return false;
                  
                  int v;
                  if (!_consume_kv_value_number())
                     return false;
                  
                  if (v <= 0) {
                     _report_error(
                        loc,
                        "you cannot use a zero or negative string length (%%<%s=%d%%> in options %%<%s%%>", this->current_key.data(),
                        v,
                        this->heritable_name.c_str()
                     );
                     return false;
                  }
                  dst.length = v;
                  
                  token = pragma_lex(&data, &loc); // consume potential comma
               } else if (key == "with_terminator") {
                  dst.with_terminator = true;
                  
                  token = pragma_lex(&data, &loc);
                  if (token == CPP_EQ) {
                     _report_error(
                        loc,
                        "unexpected key/value pair separator (%%<=%%>) found (key %%<%s%%> does not take a value) in options %%<%s%%>",
                        this->current_key.data(),
                        this->heritable_name.c_str()
                     );
                     return false;
                  }
                  //
                  // Else, fall through.
                  //
               } else {
                  _report_unrecognized_key(loc, "string");
                  
                  token = pragma_lex(&data, loc);
                  if (token == CPP_EQ) {
                     token = pragma_lex(&data, loc);
                     switch (token) {
                        case CPP_NUMBER:
                        case CPP_STRING:
                           break;
                        default:
                           _report_error(loc, "unexpected token found after key/value pair separator for key %%<%s%%> in options %%<%s%%>", this->current_key.data(), this->heritable_name.c_str());
                           return false;
                     }
                     token = pragma_lex(&data, loc); // consume potential comma
                  }
                  //
                  // Else, fall through.
                  //
               }
               
               if (token == CPP_CLOSE_PAREN)
                  break;
               if (token == CPP_COMMA) {
                  token = pragma_lex(&data, &loc);
                  continue;
               }
               
               _report_error(loc, "expected %%<,%%> or %%<)%%> while parsing options %%<%s%%>", this->heritable_name.c_str());
               return false;
            } while (true);
            
            return true;
         }
   };
}

namespace bitpacking {
   const heritable_options* heritable_options_stockpile::options_by_name(std::string_view name) const {
      static_assert(false, "TODO");
      return nullptr;
   };
   
   void heritable_options_stockpile::handle_pragma(cpp_reader* reader) {
      heritable_options loaded;
      std::string       loaded_name;
      
      location_t loc;
      tree       data;
      
      if (pragma_lex(&data, &loc) != CPP_NAME) {
         _report_error(loc, "expected a heritable type (%%<integer%%> or %%<string%%>)");
         return;
      }
      {
         std::string_view type = IDENTIFIER_POINTER(data);
         if (type == "integer") {
            loaded.data.emplace<heritable_options::integral_data>();
         } else if (type == "string") {
            loaded.data.emplace<heritable_options::string_data>();
         } else {
            _report_error(loc, "unknown heritable type (%%<integer%%> or %<string%%> expected; %%<%s%%> found)", type.data());
            return;
         }
      }
      
      if (pragma_lex(&data, &loc) != CPP_STRING) {
         _report_error(loc, "expected a name (string literal) for this set of heritable options");
         return;
      }
      loaded_name = TREE_STRING_POINTER(data);
      if (loaded_name.empty()) {
         _report_error(loc, "a set of heritable options cannot have a blank name");
         return;
      }
      
      auto kv_set = gcc_helpers::extract_pragma_kv_args("#pragma lu-bitpack heritable");
      
      //
      // Options common to all types:
      //
      if (auto it = kv_set.find("pre_pack"); it != kv_set.end()) {
         switch (it->second.index()) {
            case 0:
               _report_error(loc, "value missing for key %%<%s%%> in option set %%<%s%%>", it->first.c_str(), loaded_name.c_str());
               return;
            case 1:
               casted->transforms.pre_pack = std::get<1>(it->second);
               break;
            case 2:
               casted->transforms.pre_pack = std::get<2>(it->second);
               break;
            default:
               _report_error(loc, "invalid value specified for key %%<%s%%> in option set %%<%s%%>", it->first.c_str(), loaded_name.c_str());
               return;
         }
      }
      if (auto it = kv_set.find("post_unpack"); it != kv_set.end()) {
         switch (it->second.index()) {
            case 0:
               _report_error(loc, "value missing for key %%<%s%%> in option set %%<%s%%>", it->first.c_str(), loaded_name.c_str());
               return;
            case 1:
               casted->transforms.post_unpack = std::get<1>(it->second);
               break;
            case 2:
               casted->transforms.post_unpack = std::get<2>(it->second);
               break;
            default:
               _report_error(loc, "invalid value specified for key %%<%s%%> in option set %%<%s%%>", it->first.c_str(), loaded_name.c_str());
               return;
         }
      }
      //
      // Type-specific options:
      //
      if (auto* casted = std::get_if<heritable_options::integral_data>(&loaded.data)) {
         if (auto it = kv_set.find("bitcount"); it != kv_set.end()) {
            if (!std::holds_alternative<intmax_t>(it->second)) {
               _report_error(loc, "bitcount for option set %%<%s%%> is not a valid integer literal", loaded_name.c_str());
               return;
            }
            auto v = std::get<intmax_t>(it->second);
            if (v <= 0) {
               _report_error(loc, "bitcount for integer option set %%<%s%%> is zero or negative", loaded_name.c_str());
               return;
            }
            casted->bitcount = v;
         }
         if (auto it = kv_set.find("min"); it != kv_set.end()) {
            if (!std::holds_alternative<intmax_t>(it->second)) {
               _report_error(loc, "min for integer option set %%<%s%%> is not a valid integer literal", loaded_name.c_str());
               return;
            }
            auto v = std::get<intmax_t>(it->second);
            casted->min = v;
         }
         if (auto it = kv_set.find("max"); it != kv_set.end()) {
            if (!std::holds_alternative<intmax_t>(it->second)) {
               _report_error(loc, "max for integer option set %%<%s%%> is not a valid integer literal", loaded_name.c_str());
               return;
            }
            auto v = std::get<intmax_t>(it->second);
            casted->max = v;
         }
      } else if (auto* casted = std::get_if<heritable_options::string_data>(&loaded.data)) {
         if (auto it = kv_set.find("length"); it != kv_set.end()) {
            if (!std::holds_alternative<intmax_t>(it->second)) {
               _report_error(loc, "length for string option set %%<%s%%> is not a valid integer literal", loaded_name.c_str());
               return;
            }
            auto v = std::get<intmax_t>(it->second);
            if (v <= 0) {
               _report_error(loc, "length for string option set %%<%s%%> is zero or negative", loaded_name.c_str());
               return;
            }
            casted->length = v;
         }
         if (auto it = kv_set.find("with_terminator"); it != kv_set.end()) {
            if (!std::holds_alternative<std::monostate>(it->second)) {
               _report_warning(loc, "string option set %%<%s%%> key %%<%s%%> does not take a value; the value seen will be ignored", loaded_name.c_str(), it->first.c_str());
            }
            casted->with_terminator = true;
         }
      }
      
      //
      // Try to store the option set:
      //
      auto& item = this->_heritables[loaded_name];
      if (!std::holds_alternative<std::monostate>(item.data)) {
         if (item.data.index() == loaded.data.index()) {
            _report_warning(loc, "redefinition of heritable option set %%<%s%%>", loaded_name.c_str());
         } else {
            _report_error(loc, "attempted redefinition of heritable option set %%<%s%%> with a different value type; ignoring", loaded_name.c_str());
            return;
         }
      }
      item = loaded;
   }
}