#include "codegen/decl_descriptor.h"
#include <algorithm>
#include <cassert>
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/container.h"

namespace codegen {
   void decl_descriptor::_compute_types() {
      assert(!this->types.basic_type.empty());
      
      if (this->options.is_string()) {
         assert(this->types.basic_type.is_array());
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
      
      if (this->options.is_transforms()) {
         this->types.serialized = this->options.transform_options().transformed_type;
         this->types.transformations.push_back(this->types.serialized);
         
         auto& dictionary = decl_dictionary::get_fast();
         auto  type       = this->types.serialized;
         do {
            type = dictionary.type_transforms_into(type);
            if (!type.empty()) {
               this->types.transformations.push_back(type);
            }
         } while (!type.empty());
      }
   }

   decl_descriptor::decl_descriptor(gcc_wrappers::decl::field decl) {
      assert(!decl.empty());
      this->decl = decl;
      this->types.basic_type = decl.value_type();
      this->has_any_errors = !this->options.load(decl);
      this->_compute_types();
   }
   decl_descriptor::decl_descriptor(gcc_wrappers::decl::param decl) {
      assert(!decl.empty());
      this->decl = decl;
      this->types.basic_type = decl.value_type();
      if (this->types.basic_type.is_pointer()) {
         //
         // Special-case: when we generate whole-struct functions, those 
         // functions will take the struct to serialize as a pointer argument. 
         // We need to strip the pointer.
         //
         this->types.basic_type = this->types.basic_type.remove_pointer();
      }
      this->has_any_errors = !this->options.load(decl);
      this->_compute_types();
   }
   decl_descriptor::decl_descriptor(gcc_wrappers::decl::variable decl) {
      assert(!decl.empty());
      this->decl = decl;
      this->types.basic_type = decl.value_type();
      this->has_any_errors = !this->options.load(decl);
      this->_compute_types();
   }
   
   std::vector<const decl_descriptor*> decl_descriptor::members_of_serialized() const {
      auto type = this->types.serialized;
      assert(type.is_record() || type.is_union());
      
      auto& dictionary = decl_dictionary::get_fast();
      
      std::vector<const decl_descriptor*> out;
      if (type.is_record()) {
         type.as_container().for_each_referenceable_field([&dictionary, &out](gcc_wrappers::decl::field decl) {
            const auto& item = dictionary.get_or_create_descriptor(decl);
            out.push_back(&item);
         });
      } else if (type.is_union()) {
         type.as_container().for_each_field([&dictionary, &out](gcc_wrappers::decl::field decl) {
            const auto& item = dictionary.get_or_create_descriptor(decl);
            out.push_back(&item);
         });
      }
      return out;
   }
   
   size_t decl_descriptor::serialized_type_size_in_bits() const {
      if (options.is_buffer())
         return options.buffer_options().bytecount * 8;
      
      if (options.is_integral())
         return options.integral_options().bitcount;
      
      if (options.is_string())
         return options.string_options().length * 8;
      
      auto type = this->types.serialized;
      if (type.is_record()) {
         size_t total = 0;
         for(auto* desc : this->members_of_serialized()) {
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
         size_t total = 0;
         for(auto* desc : this->members_of_serialized()) {
            size_t item = desc->serialized_type_size_in_bits();
            for(auto e : desc->array.extents) {
               if (e == vla_extent)
                  item *= 0;
               else
                  item *= e;
            }
            if (total < item)
               total = item;
         }
         return total;
      }
      
      return 0;
   }
   
   bool decl_descriptor::is_or_contains_defaulted() const {
      if (this->options.default_value_node != NULL_TREE)
         return true;
      auto type = this->types.serialized;
      if (type.is_record()) {
         auto members = this->members_of_serialized();
         for(const auto* m : members)
            if (m->is_or_contains_defaulted())
               return true;
      }
      return false;
   }
   
   //
   // decl_dictionary
   //

   const decl_descriptor& decl_dictionary::get_or_create_descriptor(gcc_wrappers::decl::field decl) {
      assert(!decl.empty());
      auto& item = this->_data[decl];
      if (!item.get())
         item = std::make_unique<decl_descriptor>(decl);
      return *item.get();
   }
   const decl_descriptor& decl_dictionary::get_or_create_descriptor(gcc_wrappers::decl::param decl) {
      assert(!decl.empty());
      auto& item = this->_data[decl];
      if (!item.get())
         item = std::make_unique<decl_descriptor>(decl);
      return *item.get();
   }
   const decl_descriptor& decl_dictionary::get_or_create_descriptor(gcc_wrappers::decl::variable decl) {
      assert(!decl.empty());
      auto& item = this->_data[decl];
      if (!item.get())
         item = std::make_unique<decl_descriptor>(decl);
      return *item.get();
   }
   
   gcc_wrappers::type::base decl_dictionary::type_transforms_into(gcc_wrappers::type::base type) {
      auto& item = this->_transformations[type];
      if (item.has_value())
         return *item;
      
      bitpacking::data_options::computed options;
      options.load(type);
      
      if (options.is_transforms()) {
         item = options.transform_options().transformed_type;
         return *item;
      }
      return {};
   }
   
}