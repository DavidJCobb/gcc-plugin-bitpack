#include "bitpacking/data_options.h"

namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace bitpacking::data_options {
   
   //
   // requested
   //
   
   void requested::from_decl(gw:decl::field decl) {
      auto list = decl.attributes();
      for(auto it : list) {
         auto k = it->first;
         auto v = it->second;
         if (TREE_CODE(k) != IDENTIFIER_NODE)
            continue;
         
         std::string_view key = IDENTIFIER_POINTER(k);
         if (key == "lu_bitpack_omit") {
            this->do_not_serialize = true;
            continue;
         }
         if (key == "lu_bitpack_bitcount") {
            if (TREE_CODE(v) != INTEGER_CST) {
               throw; // TODO
            }
            auto value = TREE_INT_CST_LOW(v);
            if (value <= 0) {
               throw; // TODO
            }
            if (value > sizeof(size_t) * 8) {
               // TODO: Warn on suspiciously large size
            }
            this->integral.bitcount = TREE_INT_CST_LOW(v);
            continue;
         }
         if (key == "lu_bitpack_funcs") {
            //
            // TODO: extract and parse string constant to store transform func identifiers
            //
            continue;
         }
         if (key == "lu_bitpack_inherit") {
            //
            // TODO: extract
            //
            continue;
         }
         if (key == "lu_bitpack_range") {
            if (v == NULL_TREE) {
               throw; // TODO: insufficient args
            }
            auto v_min = v;
            auto v_max = TREE_CHAIN(v);
            if (v_max == NULL_TREE) {
               throw; // TODO: insufficient args
            }
            if (TREE_CODE(v_min) != INTEGER_CST) {
               throw; // TODO
            }
            if (TREE_CODE(v_max) != INTEGER_CST) {
               throw; // TODO
            }
            this->integral.min = TREE_INT_CST_LOW(v_min);
            this->integral.max = TREE_INT_CST_LOW(v_max);
            continue;
         }
         if (attr_name == "lu_bitpack_string") {
            if (!decl.value_type().is_array()) {
               throw; // TODO: not a string
            }
            if (!gcc_helpers::type_is_char(decl_type)) {
               throw; // TODO: not a string
            }
            this->as_string = true;
            
            if (v == NULL_TREE) {
               continue;
            }
            
            static_assert(false, "TODO: parse TREE_VALUE(v) as a constant string");
            // use `handle_kv_string`
            
            auto value = TREE_VALUE(argument);
            auto str   = gcc_helpers::extract_string_constant_from_tree(value);
            
            std::optional<int> length;
            bool null_terminated = false;
            
            const auto params = std::array{
               kv_string_param{
                  .name      = "length",
                  .has_param = true,
                  .int_param = true,
                  .handler   = [&length](std::string_view v, int vi) {
                     length = vi;
                     return true;
                  },
               },
               kv_string_param{
                  .name      = "with-terminator",
                  .has_param = false,
                  .handler   = [&null_terminated](std::string_view v, int vi) {
                     null_terminated = true;
                     return true;
                  },
               },
            };
            if (handle_kv_string(str, params)) {
               if (length.has_value()) {
                  if (length < 0) {
                     throw; // TODO: zero or negative length
                  }
                  this->string.length   = length;
               }
               this->null_terminated = null_terminated;
            }
            continue;
         }
         // Unrecognized/irrelevant attr.
      }
   }
   
   bool requested::requests_integral() const {
      //
      // TODO: Check inheritance.
      //
      if (this->integral.bitcount.has_value())
         return true;
      if (this->integral.min.has_value())
         return true;
      if (this->integral.max.has_value())
         return true;
      return false;
   }
   bool requested::requests_string() const {
      //
      // TODO: Check inheritance.
      //
      if (this->string.length.has_value())
         return true;
      if (this->string.null_terminated.has_value())
         return true;
      return false;
   }
   
   //
   // computed
   //
   
   void computed::_from_type(gcc_wrappers::type type, const requested& src) {
      if (type.is_record()) {
         assert(!src.requests_integral());
         assert(!src.requests_string());
      } else if (type.is_pointer()) {
         assert(!src.requests_string());
      }
      //
      // Disallow contradictions:
      //
      if (src.as_opaque_buffer) {
         assert(!src.requests_integral());
         assert(!src.requests_string());
      } else if (src.requests_integral()) {
         assert(!src.requests_string());
      } else if (src.requests_string()) {
         assert(!src.requests_integral());
      }
      
      this->transform = src.transform;
      if (src.as_opaque_buffer) {
         this->packing = computed_buffer_options{
            .bytecount = type.size_in_bytes(),
         };
         return;
      }
      if (src.requests_integral()) {
         static_assert(false, "TODO");
      } else if (src.requests_string()) {
         static_assert(false, "TODO");
      }
   }
   
   computed::computed(gcc_wrappers::type type, const requested& src) {
      this->_from_type(type, src);
   }
   computed(gcc_wrappers::decl::field decl, const requested& src) {
      auto vt = decl.value_type();
      while (vt.is_array()) {
         vt = vt.array_value_type();
      }
      this->_from_type(vt, src);
      
      if (std::holds_alternative<computed_integral_options>(this->packing)) {
         if (decl.is_bitfield()) {
            
         }
      }
   }
}