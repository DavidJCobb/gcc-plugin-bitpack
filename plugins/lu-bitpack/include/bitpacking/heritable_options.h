#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include "lu/singleton.h"
#include "bitpacking/member_options.h"

#include <gcc-plugin.h>
#include <plugin-version.h>
#include <c-family/c-pragma.h>

namespace bitpacking {
   /*
      #pragma lu-bitpack heritable integer "$game-language" ( min = 0, max = 7 )
      #pragma lu-bitpack heritable integer "$global-item-id" ( max = 511 )
      #pragma lu-bitpack heritable string "$player-name" ( length = 7 )
   */
   class heritable_options {
      public:
         struct integral_data {
            std::optional<size_t> bitcount;
            std::optional<size_t> min;
            std::optional<size_t> max;
         };
         struct string_data {
            std::optional<size_t> length;
            std::optional<bool>   with_terminator;
         };
      
      public:
         std::variant<
            std::monostate,
            integral_data,
            string_data
         > data;
         //
         // Options common to, and heritable for, all data types:
         //
         struct {
            std::string pre_pack;
            std::string post_unpack;
         } transforms;
   };

   class heritable_options_stockpile;
   class heritable_options_stockpile : public lu::singleton<heritable_options_stockpile> {
      protected:
         std::unordered_map<std::string, heritable_options> _heritables;
         
      public:
         const heritable_options* options_by_name(std::string_view) const;
         
         void handle_pragma(cpp_reader* reader);
   };
}