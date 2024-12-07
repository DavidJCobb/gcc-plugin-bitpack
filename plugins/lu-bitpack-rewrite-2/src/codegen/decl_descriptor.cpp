#include <algorithm>
#include <cassert>
#include <memory> // GCC headers fuck up string-related STL identifiers that <memory> depends on
#include "codegen/decl_descriptor.h"
#include "codegen/decl_dictionary.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/container.h"
namespace gw {
   using namespace gcc_wrappers;
}
namespace typed_options {
   using namespace bitpacking::typed_data_options::computed;
}

namespace codegen {
   void decl_descriptor::_compute_types() {
      if (this->options.is<typed_options::string>()) {
         assert(this->types.basic_type.is_array());
         //
         // For strings, we strip all array ranks except for the innermost one, 
         // and we consider the innermost array rank a "string." For example, 
         // the type `char foo[5][8]` is an array of five strings.
         //
         auto array_type = this->types.basic_type.as_array();
         auto value_type = array_type.value_type();
         while (value_type.is_array()) {
            if (auto opt = array_type.extent(); opt.has_value()) {
               this->array.extents.push_back(*opt);
            } else {
               this->array.extents.push_back(vla_extent);
            }
            array_type = value_type.as_array();
            value_type = array_type.value_type();
         }
         this->types.innermost  = value_type;
         this->types.serialized = array_type;
      } else {
         //
         // For non-strings, we strip all array ranks.
         //
         auto type = this->types.basic_type;
         while (type.is_array()) {
            if (auto opt = type.as_array().extent(); opt.has_value()) {
               this->array.extents.push_back(*opt);
            } else {
               this->array.extents.push_back(vla_extent);
            }
            type = type.as_array().value_type();
         }
         this->types.innermost  = type;
         this->types.serialized = type;
      }
      
      if (this->options.is<typed_options::transformed>()) {
         this->types.serialized = this->options.as<typed_options::transformed>().transformed_type;
         this->types.transformations.push_back(*this->types.serialized);
         
         auto& dictionary = decl_dictionary::get_fast();
         
         gw::type::optional_base type = this->types.serialized;
         do {
            type = dictionary.type_transforms_into(*type);
            if (type) {
               this->types.transformations.push_back(*type);
            }
         } while (type);
      }
   }

   decl_descriptor::decl_descriptor(gw::decl::field decl)
   :
      decl(decl),
      types({
         .basic_type = decl.value_type(),
      })
   {
      this->options.load(decl);
      this->has_any_errors = !this->options.valid();
      this->_compute_types();
   }
   decl_descriptor::decl_descriptor(gw::decl::param decl)
   :
      decl(decl),
      types({
         .basic_type = decl.value_type(),
      })
   {
      if (this->types.basic_type.is_pointer()) {
         //
         // Special-case: when we generate whole-struct functions, those 
         // functions will take the struct to serialize as a pointer argument. 
         // We need to strip the pointer.
         //
         this->types.basic_type = this->types.basic_type.remove_pointer();
      }
      this->options.load(decl);
      this->has_any_errors = !this->options.valid();
      this->_compute_types();
   }
   decl_descriptor::decl_descriptor(gw::decl::variable decl)
   :
      decl(decl),
      types({
         .basic_type = decl.value_type(),
      })
   {
      this->options.load(decl);
      this->has_any_errors = !this->options.valid();
      this->_compute_types();
   }
   
   std::vector<const decl_descriptor*> decl_descriptor::members_of_serialized() const {
      auto type = *this->types.serialized;
      assert(type.is_container());
      
      auto& dictionary = decl_dictionary::get_fast();
      
      std::vector<const decl_descriptor*> out;
      if (type.is_record()) {
         type.as_container().for_each_referenceable_field([&dictionary, &out](gw::decl::field decl) {
            const auto& item = dictionary.describe(decl);
            out.push_back(&item);
         });
      } else if (type.is_union()) {
         type.as_container().for_each_field([&dictionary, &out](gw::decl::field decl) {
            const auto& item = dictionary.describe(decl);
            out.push_back(&item);
         });
      }
      return out;
   }
   
   size_t decl_descriptor::serialized_type_size_in_bits() const {
      if (this->options.is<typed_options::buffer>())
         return options.as<typed_options::buffer>().bytecount * 8;
      
      if (this->options.is<typed_options::integral>())
         return options.as<typed_options::integral>().bitcount;
      
      if (this->options.is<typed_options::string>())
         return options.as<typed_options::string>().length * 8;
      
      auto type = *this->types.serialized;
      if (type.is_record()) {
         size_t total = 0;
         for(auto* desc : this->members_of_serialized()) {
            if (desc->options.is_omitted)
               continue;
            size_t item = desc->serialized_type_size_in_bits();
            for(auto e : desc->array.extents) {
               if (e == vla_extent)
                  item *= 0;
               else
                  item *= e;
            }
            total += item;
         }
         return total;
      }
      if (type.is_union()) {
         size_t largest = 0;
         for(auto* desc : this->members_of_serialized()) {
            if (desc->options.is_omitted)
               continue;
            size_t item = desc->serialized_type_size_in_bits();
            for(auto e : desc->array.extents) {
               if (e == vla_extent)
                  item *= 0;
               else
                  item *= e;
            }
            if (largest < item)
               largest = item;
         }
         return largest;
      }
      
      return 0;
   }
   
   size_t decl_descriptor::unpacked_single_size_in_bits() const {
      if (this->options.is<typed_options::buffer>())
         return options.as<typed_options::buffer>().bytecount * 8;
      
      auto type = *this->types.serialized;
      if (this->options.is<typed_options::string>()) {
         if (type.is_array()) {
            auto array_type = type.as_array();
            auto value_type = array_type.value_type();
            return *array_type.extent() * value_type.size_in_bits();
         }
      } else if (this->options.is<typed_options::transformed>()) {
         type = *this->types.innermost;
      }
      return type.size_in_bits();
   }
   
   bool decl_descriptor::is_or_contains_defaulted() const {
      if (this->options.default_value)
         return true;
      if (this->types.serialized->is_record()) {
         auto members = this->members_of_serialized();
         for(const auto* m : members)
            if (m->is_or_contains_defaulted())
               return true;
      }
      return false;
   }
}