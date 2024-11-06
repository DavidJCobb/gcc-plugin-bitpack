#include "bitpacking/heritable_options.h"
#include "lu/strings/printf_string.h"
#include "gcc_helpers/extract_pragma_kv_args.h"
#include <tree.h>
#include <diagnostic.h>

namespace {
   template<typename... Args>
   void _report_error(location_t loc, const char* fmt, Args&&... args) {
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
}

namespace bitpacking {
   const heritable_options* heritable_options_stockpile::options_by_name(std::string_view name) const {
      auto it = this->_heritables.find(std::string(name));
      if (it == this->_heritables.end())
         return nullptr;
      return &it->second;
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
      
      auto kv_set = gcc_helpers::extract_pragma_kv_args("#pragma lu-bitpack heritable", reader);
      
      //
      // Options common to all types:
      //
      if (auto it = kv_set.find("pre_pack"); it != kv_set.end()) {
         switch (it->second.index()) {
            case 0:
               _report_error(loc, "value missing for key %%<%s%%> in option set %%<%s%%>", it->first.c_str(), loaded_name.c_str());
               return;
            case 1:
               loaded.transforms.pre_pack = std::get<1>(it->second);
               break;
            case 2:
               loaded.transforms.pre_pack = std::get<2>(it->second);
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
               loaded.transforms.post_unpack = std::get<1>(it->second);
               break;
            case 2:
               loaded.transforms.post_unpack = std::get<2>(it->second);
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
      loaded.name = std::move(loaded_name);
      item = loaded;
   }
}