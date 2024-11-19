#include "codegen/instructions/single.h"
#include <cassert>
#include "attribute_handlers/helpers/type_transitively_has_attribute.h"
#include "codegen/instruction_generation_context.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/local_block.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/attribute.h"
#include "gcc_wrappers/builtin_types.h"
#include "gcc_wrappers/value.h"
#include "bitpacking/global_options.h"
#include "basic_global_state.h"

namespace gw {
   using namespace gcc_wrappers;
}

namespace codegen::instructions {
   bool single::is_omitted_and_defaulted() const {
      bool omitted   = false;
      bool defaulted = false;
      for(auto& segm : this->value.segments) {
         auto  pair = segm.descriptor();
         auto* desc = pair.read;
         if (desc == nullptr)
            continue;
         if (desc->options.omit_from_bitpacking)
            omitted = true;
         if (desc->is_or_contains_defaulted())
            defaulted = true;
         
         if (omitted && defaulted)
            return true;
      }
      return omitted && defaulted;
   }
   
   static gw::expr::base _default_a_string(
      const value_path& value_path,
      value_pair        value
   ) {
      auto& bgs = basic_global_state::get();
      assert(!bgs.builtin_functions.memcpy.empty());
      assert(!bgs.builtin_functions.memset.empty());
      
      const auto& ty      = gw::builtin_types::get();
      const auto& options = value_path.bitpacking_options();
      
      // Must be called for a string-type default value.
      assert(options.default_value_node != NULL_TREE);
      assert(TREE_CODE(options.default_value_node) == STRING_CST);
      
      // Must be called for a to-be-defaulted string.
      assert(value.read.value_type().is_array());
      
      gw::type::array array_type = value.read.value_type().as_array();
      gw::type::base  char_type  = array_type.value_type();
      
      auto      str     = gw::expr::string_constant::from_untyped(options.default_value_node);
      gw::value literal = str.to_string_literal(char_type);
      
      size_t char_size   = char_type.empty() ? 1 : char_type.size_in_bytes();
      size_t string_size = str.size_of();
      size_t array_size  = *array_type.extent();
      
      if (options.has_attr_nonstring) {
         string_size -= 1; // exclude null terminator
      }
      
      auto call_memcpy = gw::expr::call(
         bgs.builtin_functions.memcpy,
         // args:
         value.read.convert_array_to_pointer(),
         literal,
         gw::expr::integer_constant(ty.size, string_size * char_size)
      );
      if (array_size <= string_size) {
         return call_memcpy;
      }
      auto call_memset = gw::expr::call(
         bgs.builtin_functions.memset,
         // args:
         value.read.access_array_element(
            gw::expr::integer_constant(ty.basic_int, string_size)
         ).address_of(),
         gw::expr::integer_constant(ty.basic_int, 0),
         gw::expr::integer_constant(ty.size, (array_size - string_size) * char_size)
      );
      
      gw::expr::local_block block;
      block.statements().append(call_memcpy);
      block.statements().append(call_memset);
      return block;
   }
   
   /*virtual*/ expr_pair single::generate(const instruction_generation_context& ctxt) const {
      const auto& ty = gw::builtin_types::get();
      
      auto        value   = this->value.as_value_pair();
      const auto& options = this->value.bitpacking_options();
      const auto& global  = basic_global_state::get().global_options.computed;
      
      //
      // Handle values that are omitted and defaulted.
      //
      if (options.omit_from_bitpacking) {
         if (options.default_value_node == NULL_TREE) {
            return expr_pair{
               .read = gw::expr::base::from_untyped(build_empty_stmt(UNKNOWN_LOCATION)),
               .save = gw::expr::base::from_untyped(build_empty_stmt(UNKNOWN_LOCATION)),
            };
         }
         //
         // Structs can't be defaulted, and this class should never be used 
         // to denote serialization of an array (unless it's a defaulted 
         // string), so we can just assume that we're operating on a string 
         // or a scalar.
         //
         if (TREE_CODE(options.default_value_node) == STRING_CST) {
            return expr_pair{
               .read = _default_a_string(this->value, value),
               .save = gw::expr::base::from_untyped(build_empty_stmt(UNKNOWN_LOCATION)),
            };
         }
         //
         // Non-string defaults.
         //
         switch (TREE_CODE(options.default_value_node)) {
            case INTEGER_CST:
            case REAL_CST:
               break;
            default:
               assert(false && "unreachable");
         }
         gw::value src_value;
         src_value.set_from_untyped(options.default_value_node);
         return expr_pair{
            .read = gw::expr::assign(value.read, src_value),
            .save = gw::expr::base::from_untyped(build_empty_stmt(UNKNOWN_LOCATION)),
         };
      }
      
      //
      // Handle values that aren't omitted-and-defaulted.
      //
      
      if (options.kind == bitpacking::member_kind::boolean) {
         return expr_pair{
            .read = gw::expr::assign(
               value.read,
               gw::expr::call(
                  global.functions.read.boolean,
                  // args:
                  ctxt.state_ptr.read
               )
            ),
            .save = gw::expr::call(
               global.functions.save.boolean,
               // args:
               ctxt.state_ptr.save,
               value.save
            )
         };
      }
      
      if (options.kind == bitpacking::member_kind::buffer) {
         auto bytecount = options.buffer_options().bytecount;
         auto size_arg  = gw::expr::integer_constant(ty.uint16, bytecount);
         return expr_pair{
            .read = gw::expr::call(
               global.functions.read.buffer,
               // args:
               ctxt.state_ptr.read,
               value.read.address_of(),
               size_arg
            ),
            .save = gw::expr::call(
               global.functions.save.buffer,
               // args:
               ctxt.state_ptr.save,
               value.save.address_of(),
               size_arg
            )
         };
      }
      
      if (options.kind == bitpacking::member_kind::integer) {
         auto type = value.read.value_type();
         gw::decl::function read_func;
         gw::decl::function save_func;
         {
            auto vt_canonical = type.canonical();
            if (vt_canonical == ty.uint8 || vt_canonical == ty.basic_char) {
               read_func = global.functions.read.u8;
               save_func = global.functions.save.u8;
            } else if (vt_canonical == ty.uint16) {
               read_func = global.functions.read.u16;
               save_func = global.functions.save.u16;
            } else if (vt_canonical == ty.uint32) {
               read_func = global.functions.read.u32;
               save_func = global.functions.save.u32;
            } else if (vt_canonical == ty.int8) {
               read_func = global.functions.read.s8;
               save_func = global.functions.save.s8;
            } else if (vt_canonical == ty.int16) {
               read_func = global.functions.read.s16;
               save_func = global.functions.save.s16;
            } else if (vt_canonical == ty.int32) {
               read_func = global.functions.read.s32;
               save_func = global.functions.save.s32;
            }
         }
         assert(!read_func.empty());
         assert(!save_func.empty());
         
         auto ic_bitcount = gw::expr::integer_constant(ty.uint8,           options.integral_options().bitcount);
         auto ic_min      = gw::expr::integer_constant(type.as_integral(), options.integral_options().min);
         
         expr_pair out;
         {  // Read
            gw::value to_assign = gw::expr::call(
               read_func,
               // args:
               ctxt.state_ptr.read,
               ic_bitcount
            );
            if (options.integral_options().min != 0) {
               //
               // If a field's range of valid values is [a, b], then serialize it as (v - a), 
               // and then unpack it as (v + a). This means we don't need a sign bit when we 
               // serialize negative numbers, and it also means that ranges that start above 
               // zero don't waste the low bits.
               //
               to_assign = to_assign.add(ic_min);
            }
            out.read = gw::expr::assign(value.read, to_assign);
         }
         {  // Save
            auto to_save = value.save;
            if (options.integral_options().min != 0)
               to_save = to_save.sub(ic_min);
            out.save = gw::expr::call(
               save_func,
               // args:
               ctxt.state_ptr.save,
               to_save,
               ic_bitcount
            );
         }
         return out;
      }
      
      if (options.kind == bitpacking::member_kind::pointer) {
         auto type = value.read.value_type();
         gw::decl::function read_func;
         gw::decl::function save_func;
         switch (type.size_in_bits()) {
            case 8:
               read_func = global.functions.read.u8;
               save_func = global.functions.save.u8;
               break;
            case 16:
               read_func = global.functions.read.u16;
               save_func = global.functions.save.u16;
               break;
            case 32:
               read_func = global.functions.read.u32;
               save_func = global.functions.save.u32;
               break;
            default:
               assert(false && "unsupported pointer size");
         }
         assert(!read_func.empty());
         assert(!save_func.empty());
         
         auto bitcount_arg = gw::expr::integer_constant(ty.uint8, type.size_in_bits());
         
         return expr_pair{
            .read = gw::expr::assign(
               value.read,
               gw::expr::call(
                  read_func,
                  // args:
                  ctxt.state_ptr.read,
                  bitcount_arg
               ).conversion_sans_bytecode(type)
            ),
            .save = gw::expr::call(
               save_func,
               // args:
               ctxt.state_ptr.save,
               value.save.conversion_sans_bytecode(ty.smallest_integral_for(type.size_in_bits(), false)),
               bitcount_arg
            )
         };
      }
      
      if (options.kind == bitpacking::member_kind::string) {
         auto& str_opt = options.string_options();
         
         gw::decl::function read_func;
         gw::decl::function save_func;
         if (str_opt.nonstring) {
            read_func = global.functions.read.string_ut;
            save_func = global.functions.save.string_ut;
         } else {
            read_func = global.functions.read.string_wt;
            save_func = global.functions.save.string_wt;
         }
         
         auto length_arg = gw::expr::integer_constant(ty.uint16, str_opt.length);
         return expr_pair{
            .read = gw::expr::call(
               read_func,
               // args:
               ctxt.state_ptr.read,
               value.read.convert_array_to_pointer(),
               length_arg
            ),
            .save = gw::expr::call(
               save_func,
               // args:
               ctxt.state_ptr.save,
               value.save.convert_array_to_pointer(),
               length_arg
            )
         };
      }
      
      if (options.kind == bitpacking::member_kind::structure) {
         auto type = value.read.value_type();
         assert(type.is_record());
         
         auto func = ctxt.get_whole_struct_functions_for(type.as_record());
         assert(!func.read.empty());
         assert(!func.save.empty());
         
         return expr_pair{
            .read = gw::expr::call(
               func.read,
               // args:
               ctxt.state_ptr.read,
               value.read.address_of()
            ),
            .save = gw::expr::call(
               func.save,
               // args:
               ctxt.state_ptr.save,
               value.save.address_of()
            )
         };
      }
      
      assert(false && "unreachable");
   }
}