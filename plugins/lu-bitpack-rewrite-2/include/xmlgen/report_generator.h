#pragma once
#include <memory>
#include <string>
#include <vector>
#include "codegen/stats/c_type.h"
#include "codegen/stats/category.h"
#include "codegen/stats/sector.h"
#include "codegen/decl_descriptor_pair.h"
#include "xmlgen/xml_element.h"
#include "gcc_wrappers/type/base.h"

namespace bitpacking {
   class data_options;
}
namespace codegen {
   namespace instructions {
      class base;
      class container;
      
      class array_slice;
      class padding;
      class single;
      class transform;
      class union_switch;
   }
   class stats_gatherer;
   class value_path;
   class whole_struct_function_dictionary;
}

namespace xmlgen {
   class report_generator {
      public:
         using owned_element = std::unique_ptr<xml_element>;
      
      protected:
         struct category_info {
            std::string              name;
            codegen::stats::category stats;
         };
         struct sector_info {
            codegen::stats::sector stats;
            owned_element          instructions;
         };
         struct type_info {
            type_info(gcc_wrappers::type::base t) : stats(t) {}
            
            codegen::stats::c_type stats;
            owned_element          instructions;
         };
      
         std::vector<category_info> _categories;
         std::vector<sector_info>   _sectors;
         std::vector<type_info>     _types;
         
         // Used to give each loop variable a unique name and ID. We gather 
         // these up before generating XML. Each variable's ID is its index 
         // in this list.
         std::vector<codegen::decl_descriptor_pair> _loop_variables;
         
         // Similarly, used to give each transformed value a unique name.
         std::vector<codegen::decl_descriptor_pair> _transformed_values;
         
      protected:
         category_info& _get_or_create_category_info(std::string_view name);
         type_info& _get_or_create_type_info(gcc_wrappers::type::base);
         
         std::string _loop_variable_to_string(codegen::decl_descriptor_pair) const;
         std::string _variable_to_string(codegen::decl_descriptor_pair) const;
         std::string _value_path_to_string(const codegen::value_path&) const;
         
         owned_element _generate(const codegen::instructions::array_slice&);
         owned_element _generate(const codegen::instructions::padding&);
         owned_element _generate(const codegen::instructions::single&);
         owned_element _generate(const codegen::instructions::transform&);
         owned_element _generate(const codegen::instructions::union_switch&);
         
         owned_element _generate(const codegen::instructions::base&);
         
         owned_element _generate_root(const codegen::instructions::container&);
         
      public:
         void process(const codegen::instructions::container& sector_root);
         void process(const codegen::whole_struct_function_dictionary&);
         void process(const codegen::stats_gatherer&);
         
         std::string bake();
   };
}