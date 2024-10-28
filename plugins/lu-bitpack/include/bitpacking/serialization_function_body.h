#pragma once
#include <string>
#include <vector>
#include "gcc_wrappers/decl/function.h"

namespace codegen {
   class serialization_item;
}

namespace codegen {
   class serialization_function_body {
      public:
         using item_list_type = std::vector<serialization_item*>;
   
      protected:
         struct array_rank_info {
            size_t deepest_rank = 0;
            //
            bool all_indices_are_u8       = true;
            bool all_indices_are_numbered = false;

            constexpr array_rank_info(const item_list_type&);

            constexpr std::string generate_forward_declarations() const;
            constexpr std::string rank_to_index_variable(size_t) const;
         };

      public:
         item_list_type     item_list; // unowned
         gcc_wrappers::type bitstream_state_pointer_type;
         gcc_wrappers::type type_to_serialize;
         struct {
            bool state_is_argument = true;
         } options;
         struct {
            gcc_wrappers::decl::function read;
            gcc_wrappers::decl::function save;
         } results;

      protected:
         constexpr std::string _generate_loop_start(const std::string& var, size_t start, const std::string& stop);
         constexpr std::string _generate_loop_start(const std::string& var, size_t start, size_t stop);
         constexpr std::string _generate_loop_start(const std::string& var, size_t start, const ast::member::array_rank& stop);

      public:
         void generate();
   };
}