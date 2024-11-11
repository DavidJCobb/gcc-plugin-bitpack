#include "codegen/descriptors/base_descriptor.h"
#include <cassert>
#include <stdexcept>
#include "lu/strings/printf_string.h"
#include "gcc_wrappers/type/array.h"
#include "basic_global_state.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace codegen {
   void base_descriptor::_init_type() {
      this->innermost_type = this->type;
      while (this->innermost_type.is_array()) {
         auto array_type = this->innermost_type.as_array();
         auto value_type = array_type.value_type();
         if (this->options.is_string() && !value_type.is_array()) {
            break;
         }
         
         size_t extent = 0;
         {
            auto extent_opt = array_type.extent();
            if (extent_opt.has_value())
               extent = *extent_opt;
            else
               this->array.is_variable_length = true;
         }
         this->array.extents.push_back(extent);
         
         this->innermost_type = value_type;
      }
   }
   
   base_descriptor::base_descriptor(gw::type::base type) {
      this->type    = type.value_type();
      
      data_options computed;
      if (!computed.load(type)) {
         auto pp      = this->type.pretty_print();
         auto message = lu::strings::printf_string(
            "unable to continue: a problem occurred while processing bitpacking options for type %<%s%>",
            pp.c_str()
         );
         throw std::runtime_error(message);
      }
      this->options = computed;
      
      this->_init_type();
   }
   
   base_descriptor::base_descriptor(gw::type::base type, const data_options& computed) {
      assert(!computed.omitted_from_bitpacking);
      assert(computed.kind != bitpacking::member_kind::none);
      assert(computed.kind != bitpacking::member_kind::array && "this value should only be used by member_descriptor_view");
      
      this->type    = type.value_type();
      this->options = computed;
      
      this->_init_type();
   }
}