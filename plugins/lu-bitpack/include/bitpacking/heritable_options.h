#pragma once
#include <optional>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <variant>
#include "lu/singleton.h"
#include "bitpacking/in_progress_options.h"

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
         using integral_data = in_progress_options::integral;
         using string_data   = in_progress_options::string;
      
      public:
         std::string name;
         in_progress_options::variant data;
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
         
         template<typename Functor> requires std::is_invocable_v<Functor, const std::string&, const heritable_options&>
         void for_each_option(Functor&& functor) const {
            for(auto& pair : this->_heritables) {
               functor(pair.first, pair.second);
            }
         }
         
         void handle_pragma(cpp_reader* reader);
   };
}