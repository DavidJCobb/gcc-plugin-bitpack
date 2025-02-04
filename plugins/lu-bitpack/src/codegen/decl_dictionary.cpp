#include "codegen/decl_dictionary.h"
#include <algorithm>
#include <cassert>
#include "bitpacking/data_options.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/container.h"
namespace gw {
   using namespace gcc_wrappers;
}
namespace typed_options {
   using namespace bitpacking::typed_data_options::computed;
}

namespace codegen {
   const decl_descriptor& decl_dictionary::describe(gw::decl::field decl) {
      auto& item = this->_data[decl];
      if (!item.get())
         item = std::make_unique<decl_descriptor>(decl);
      return *item.get();
   }
   const decl_descriptor& decl_dictionary::describe(gw::decl::param decl) {
      auto& item = this->_data[decl];
      if (!item.get())
         item = std::make_unique<decl_descriptor>(decl);
      return *item.get();
   }
   const decl_descriptor& decl_dictionary::describe(gw::decl::variable decl) {
      auto& item = this->_data[decl];
      if (!item.get())
         item = std::make_unique<decl_descriptor>(decl);
      return *item.get();
   }
   
   gcc_wrappers::type::optional_base decl_dictionary::type_transforms_into(gw::type::base type) {
      auto& item = this->_transformations[type];
      if (item)
         return *item;
      
      bitpacking::data_options options;
      options.load(type);
      
      if (options.is<typed_options::transformed>()) {
         item = options.as<typed_options::transformed>().transformed_type;
         return item;
      }
      return {};
   }
   
}