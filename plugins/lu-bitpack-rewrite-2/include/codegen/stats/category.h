#pragma once
#include <memory>
#include <string_view>
#include "codegen/stats/serializable.h"
#include "xmlgen/xml_element.h"
#include "gcc_wrappers/type/base.h"

namespace codegen::stats {
   class category : public serializable {
      public:
         category(gcc_wrappers::type::base);
      
      public:
         bool empty() const noexcept = delete;
         std::unique_ptr<xmlgen::xml_element> to_xml(std::string_view category_name) const;
   };
}