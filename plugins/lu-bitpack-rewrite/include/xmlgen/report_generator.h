#pragma once
#include <memory>
#include <string>
#include <vector>
#include "codegen/decl_pair.h"
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
   class value_path;
   class whole_struct_function_dictionary;
}

namespace xmlgen {
   class report_generator {
      public:
         using owned_element = std::unique_ptr<xml_element>;
      
      protected:
         std::vector<owned_element> _sectors;
         std::vector<owned_element> _types;
         
         // Used to give each loop variable a unique name and ID. We gather 
         // these up before generating XML. Each variable's ID is its index 
         // in this list.
         std::vector<codegen::decl_pair> _loop_variables;
         
         // Similarly, used to give each transformed value a unique name.
         std::vector<codegen::decl_pair> _transformed_values;
         
      protected:
         void _apply_x_options_to(
            xml_element&,
            const bitpacking::data_options::computed&
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
         
         std::string bake();
   };
}