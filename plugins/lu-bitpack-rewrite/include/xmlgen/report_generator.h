#pragma once
#include <memory>
#include <string>
#include <vector>
#include "codegen/decl_pair.h"
#include "xmlgen/report/c_type.h"
#include "xmlgen/report/sector.h"
#include "xmlgen/report/stats.h"
#include "xmlgen/xml_element.h"
#include "gcc_wrappers/type/base.h"

namespace bitpacking {
   namespace data_options {
      class computed;
   }
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
         using category_info = std::pair<std::string, report::stats>;
      
         std::vector<category_info>  _categories;
         std::vector<report::sector> _sectors;
         std::vector<report::c_type> _types;
         
         // Used to give each loop variable a unique name and ID. We gather 
         // these up before generating XML. Each variable's ID is its index 
         // in this list.
         std::vector<codegen::decl_pair> _loop_variables;
         
         // Similarly, used to give each transformed value a unique name.
         std::vector<codegen::decl_pair> _transformed_values;
         
      protected:
         report::stats& _get_or_create_category_info(std::string_view name);
         report::c_type& _get_or_create_type_info(gcc_wrappers::type::base);
      
         void _apply_x_options_to(
            xml_element&,
            const bitpacking::data_options::computed&
         );
         
         void _apply_type_info_to(
            xml_element& type_node,
            gcc_wrappers::type::base
         );
         
         std::string _loop_variable_to_string(codegen::decl_pair) const;
         std::string _variable_to_string(codegen::decl_pair) const;
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