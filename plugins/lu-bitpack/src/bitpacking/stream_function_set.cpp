#include "bitpacking/stream_function_set.h"
#include <stdexcept>
#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/builtin_types.h"
#include "lu/strings/printf_string.h"

namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
   
   // "Check" functions for function declarations, in a nested namespace for 
   // easier code folding. These verify a given precondition and throw if it 
   // isn't met.
   //
   namespace {
      void _check_exists(const char* noun, gw::decl::function decl) {
         if (!decl.empty())
            return;
         throw std::runtime_error(lu::strings::printf_string(
            "the %%<%s%%> function is missing",
            noun
         ));
      }
      
      void _check_prototyped(
         const char*        noun,
         gw::decl::function func_decl,
         gw::type           func_type
      ) {
         if (!func_type.is_unprototyped_function())
            return;
         throw std::runtime_error(lu::strings::printf_string(
            "the %%<%s%%> function (%%<%s%%>) is unprototyped; this is wrong",
            noun,
            func_decl.name.data()
         ));
      }
      
      void _check_return_type(
         const char*        noun,
         gw::decl::function func_decl,
         gw::type           func_type,
         gw::type           desired_return_type
      ) {
         auto rt = func_type.function_return_type();
         if (rt == desired_return_type)
            return;
         throw std::runtime_error(lu::strings::printf_string(
            "incorrect return type for the %%<%s%%> function (%%<%s%%>) (%%<%s%%> expected; %%<%s%%> found)",
            noun,
            func_decl.name().data(),
            desired_return_type.pretty_print().c_str(),
            rt.pretty_print().c_str()
         ));
      }
      
      /*
      
         Argument checks. Usage example:
         
            auto   list = type.function_arguments();
            auto   it   = list.begin();
            size_t i    = 0;
            _check_nth_argument    (noun, decl,   i,   it, stream_state_ptr_type);
            _check_nth_argument    (noun, decl, ++i, ++it, ty.uint8.add_pointer());
            _check_argument_endcap (noun, decl, ++it);
         
      */
      
      void _check_nth_argument(
         const char*             noun,
         gw::decl::function      func_decl,
         unsigned int            arg_index,
         gw::list_node::iterator arg_it,
         gw::type                desired_arg_type
      ) {
         auto arg_type = gw::type::from_untyped(arg_it->second);
         if (arg_type.empty()) {
            throw std::runtime_error(lu::strings::printf_string(
               "too few arguments for the %%<%s%%> function (%%<%s%%>) (only %u present)",
               noun,
               name.data(),
               arg_index
            ));
         }
         
         auto canonical_expected = arg_type.canonical();         // type the function expects
         auto canonical_received = desired_arg_type.canonical(); // type we intend to pass in
         //
         if (canonical_expected == canonical_received)
            return;
         //
         // Allow passing non-const values to functions that take consts.
         //
         if (canonical_expected.is_const() && !canonical_received.is_const()) {
            canonical_expected = canonical_expected.remove_const();
            if (canonical_expected == canonical_received)
               return;
         }
         
         throw std::runtime_error(lu::strings::printf_string(
            "incorrect argument %u type for the %%<%s%%> function (%%<%s%%>) (%%<%s%%> expected; %%<%s%%> found)",
            arg_index,
            noun,
            name.data(),
            desired_arg_type.pretty_print().c_str(),
            arg_type.pretty_print().c_str()
         ));
      }
      
      void _check_argument_endcap(
         const char*             noun,
         gw::decl::function      func_decl,
         gw::list_node::iterator arg_it
      ) {
         if (arg_it->second == ty.basic_void.as_untyped())
            return;
         if (arg_it->second == NULL_TREE) {
            auto arg_type = type::from_untyped(arg_it->second);
            throw std::runtime_error(lu::strings::printf_string(
               "extra argument(s) found for the %%<%s%%> function (%%<%s%%>) (first extra argument type is %%<%s%%>)",
               noun,
               name.data(),
               arg_type.pretty_print().c_str()
            ));
         }
         throw std::runtime_error(lu::strings::printf_string(
            "the %%<%s%%> function (%%<%s%%>) is varargs; this is wrong",
            noun,
            name.data()
         ));
      }
   }
}

namespace bitpacking {
   stream_function_set::stream_function_set() {
      const auto& ty = gw::builtin_types::get_fast();
      this->types.boolean     = ty.basic_bool;
      this->types.string_char = ty.basic_char;
   }
   
   void stream_function_set::check_all() const {
      const auto& ty = gw::builtin_types::get_fast();
      
      auto boolean_type     = this->types.boolean;
      auto buffer_byte_type = this->types.buffer_byte;
      auto string_char_type = this->types.string_char;
      
      auto stream_state_ptr_type = this->types.stream_state.add_pointer();
      
      // void lu_BitstreamInitialize(struct lu_BitstreamState*, uint8_t* buffer)
      {
         constexpr const char* noun = "bitstream initialize function";
         
         auto decl = this->stream_state_init;
         _check_exists(noun, decl);
         auto type = decl.function_type();
         _check_prototyped  (noun, decl, type);
         _check_return_type (noun, decl, type, ty.basic_void);
         
         auto   it = type.function_arguments().begin();
         size_t i  = 0;
         _check_nth_argument    (noun, decl,   i,   it, stream_state_ptr_type);
         _check_nth_argument    (noun, decl, ++i, ++it, ty.uint8.add_pointer());
         _check_argument_endcap (noun, decl, ++it);
      }
      
      // bitstream read
      {
         constexpr const char* op_name = "read";
         auto& set = this->read;
         
         // bool lu_BitstreamRead_bool(struct lu_BitstreamState*)
         {
            auto noun = lu::strings::printf_string("bitstream %s function for bool values", op_name);
            auto decl = set.boolean;
            _check_exists(noun, decl);
            auto type = decl.function_type();
            _check_prototyped  (noun, decl, type);
            _check_return_type (noun, decl, type, boolean_type);
            
            auto   it = type.function_arguments().begin();
            size_t i  = 0;
            _check_nth_argument    (noun, decl,   i,   it, stream_state_ptr_type);
            _check_argument_endcap (noun, decl, ++it);
         }
         
         // T lu_BitstreamRead_T(struct lu_BitstreamState*, uint8_t bitcount)
         auto _check_integer = [op_name, stream_state_ptr_type, &ty](
            const char*        noun_type,
            gw::decl::function decl,
            gw::type           result_type
         ) {
            auto noun = lu::strings::printf_string("bitstream %s function for %s values", op_name, noun_type);
            _check_exists(noun, decl);
            auto type = decl.function_type();
            _check_prototyped  (noun, decl, type);
            _check_return_type (noun, decl, type, result_type);
            
            auto   it = type.function_arguments().begin();
            size_t i  = 0;
            _check_nth_argument    (noun, decl,   i,   it, stream_state_ptr_type);
            _check_nth_argument    (noun, decl, ++i, ++it, ty.uint8);
            _check_argument_endcap (noun, decl, ++it);
         };
         
         _check_integer("uint8_t",  set.u8,  ty.uint8);
         _check_integer("uint16_t", set.u16, ty.uint16);
         _check_integer("uint32_t", set.u32, ty.uint32);
         //
         _check_integer("int8_t",  set.s8,  ty.int8);
         _check_integer("int16_t", set.s16, ty.int16);
         _check_integer("int32_t", set.s32, ty.int32);
         
         // void lu_BitstreamRead_T(struct lu_BitstreamState*, uint8_t*, uint16_t size)
         auto _check_span = [op_name, stream_state_ptr_type, &ty](
            const char*        noun_type,
            gw::decl::function decl,
            gw::type           element_type
         ) {
            auto noun = lu::strings::printf_string("bitstream %s function for %s", op_name, noun_type);
            _check_exists(noun, decl);
            auto type = decl.function_type();
            _check_prototyped  (noun, decl, type);
            _check_return_type (noun, decl, type, ty.basic_void);
            
            auto   it = type.function_arguments().begin();
            size_t i  = 0;
            _check_nth_argument    (noun, decl,   i,   it, stream_state_ptr_type);
            _check_nth_argument    (noun, decl, ++i, ++it, element_type.add_pointer());
            _check_nth_argument    (noun, decl, ++i, ++it, ty.uint16); // length
            _check_argument_endcap (noun, decl, ++it);
         };
         
         _check_span("strings with terminators", set.string_wt, string_char_type);
         _check_span("strings with optional terminators", set.string_ut, string_char_type);
         _check_span("buffers", set.buffer, buffer_byte_type);
      }
      
      // bitstream write
      {
         constexpr const char* op_name = "write";
         auto& set = this->save;
         
         // void lu_BitstreamRead_bool(struct lu_BitstreamState*, bool)
         {
            auto noun = lu::strings::printf_string("bitstream %s function for bool values", op_name);
            auto decl = set.boolean;
            _check_exists(noun, decl);
            auto type = decl.function_type();
            _check_prototyped  (noun, decl, type);
            _check_return_type (noun, decl, type, ty.basic_void);
            
            auto   it = type.function_arguments().begin();
            size_t i  = 0;
            _check_nth_argument    (noun, decl,   i,   it, stream_state_ptr_type);
            _check_nth_argument    (noun, decl,   i,   it, boolean_type);
            _check_argument_endcap (noun, decl, ++it);
         }
         
         // void lu_BitstreamWrite_T(struct lu_BitstreamState*, T, uint8_t bitcount)
         auto _check_integer = [op_name, stream_state_ptr_type, &ty](
            const char*        noun_type,
            gw::decl::function decl,
            gw::type           result_type
         ) {
            auto noun = lu::strings::printf_string("bitstream %s function for %s values", op_name, noun_type);
            _check_exists(noun, decl);
            auto type = decl.function_type();
            _check_prototyped  (noun, decl, type);
            _check_return_type (noun, decl, type, ty.basic_void);
            
            auto   it = type.function_arguments().begin();
            size_t i  = 0;
            _check_nth_argument    (noun, decl,   i,   it, stream_state_ptr_type);
            _check_nth_argument    (noun, decl, ++i, ++it, result_type);
            _check_nth_argument    (noun, decl, ++i, ++it, ty.uint8);
            _check_argument_endcap (noun, decl, ++it);
         };
         
         _check_integer("uint8_t",  set.u8,  ty.uint8);
         _check_integer("uint16_t", set.u16, ty.uint16);
         _check_integer("uint32_t", set.u32, ty.uint32);
         //
         _check_integer("int8_t",  set.s8,  ty.int8);
         _check_integer("int16_t", set.s16, ty.int16);
         _check_integer("int32_t", set.s32, ty.int32);
         
         // void lu_BitstreamRead_T(struct lu_BitstreamState*, const uint8_t*, uint16_t size)
         auto _check_span = [op_name, stream_state_ptr_type, &ty](
            const char*        noun_type,
            gw::decl::function decl,
            gw::type           element_type
         ) {
            auto noun = lu::strings::printf_string("bitstream %s function for %s", op_name, noun_type);
            _check_exists(noun, decl);
            auto type = decl.function_type();
            _check_prototyped  (noun, decl, type);
            _check_return_type (noun, decl, type, ty.basic_void);
            
            auto   it = type.function_arguments().begin();
            size_t i  = 0;
            _check_nth_argument    (noun, decl,   i,   it, stream_state_ptr_type);
            _check_nth_argument    (noun, decl, ++i, ++it, element_type.add_const().add_pointer());
            _check_nth_argument    (noun, decl, ++i, ++it, ty.uint16); // length
            _check_argument_endcap (noun, decl, ++it);
         };
         
         _check_span("strings with terminators", set.string_wt, string_char_type);
         _check_span("strings with optional terminators", set.string_ut, string_char_type);
         _check_span("buffers", set.buffer, buffer_byte_type);
      }
      
      // Done.
   }
   
   gcc_wrappers::expr::base stream_function_set::make_read_expression_for(
      gcc_wrappers::value state_pointer,
      gcc_wrappers::value item,
      const expression_options& options
   ) const {
      const auto& ty = gw::builtin_types::get_fast();
      
      auto vt = item.value_type();
      assert(!vt.is_record() && !vt.is_union());
      assert(!vt.is_const());
      
      // Handle strings.
      if (vt.is_array()) {
         const auto& o = options.string;
         assert(vt.array_value_type() == this->types.string_char);
         assert(o.max_length != std::numeric_limits<size_t>::max() && "Specify the max length!");
         
         gw::decl::function read_func;
         if (o.needs_terminator) {
            read_func = this->read.stream_wt;
         } else {
            read_func = this->read.stream_ut;
         }
         
         return gw::expr::call(
            read_func,
            // args:
            state_pointer,
            item.convert_array_to_pointer(),
            gw::expr::integer_constant(ty.uint16, o.max_length)
         );
      }
      
      // Handle buffers.
      if (vt.is_pointer()) {
         auto pointed_to = vt.remove_pointer();
         assert(pointed_to.is_record() || pointed_to.is_union());
         
         if (pointed_to != this->types.buffer_byte) {
            item = item.conversion_sans_bytecode(this->types.buffer_byte.add_pointer());
         }
         
         const auto& o = options.buffer;
         return gw::expr::call(
            this->read.buffer,
            // args:
            state_pointer,
            item,
            gw::expr::integer_constant(ty.uint16, o.bytecount)
         );
      }
      
      const auto& o = options.integral;
      assert(o.bitcount != std::numeric_limits<size_t>::max() && "Specify the integral's bitcount!");
      if (vt == this->types.boolean && o.bitcount == 1) {
         return gw::expr::assign(
            item,
            // =
            gw::expr::call(
               this->read.boolean,
               // args:
               state_pointer
            )
         );
      }
      
      gw::decl::function func;
      if (vt.is_pointer()) {
         assert(o.min == 0 && "Cannot support minimum values, offsets, etc., on pointers!");
         //
         // TODO: Support other pointer widths!
         //
         func = this->read.u32;
      } else {
         auto vt_canonical = vt.canonical();
         if (vt_canonical == ty.uint8) {
            func = this->read.u8;
         } else if (vt_canonical == ty.uint16) {
            func = this->read.u16;
         } else if (vt_canonical == ty.uint32) {
            func = this->read.u32;
         } else if (vt_canonical == ty.int8) {
            func = this->read.s8;
         } else if (vt_canonical == ty.int16) {
            func = this->read.s16;
         } else if (vt_canonical == ty.int32) {
            func = this->read.s32;
         }
      }
      assert(!func.empty());
      
      gw::value to_assign = gw::expr::call(
         func,
         // args:
         state_pointer,
         gw::expr::integer_constant(ty.uint8, o.bitcount)
      );
      //
      // Permutations made to the raw value after reading it (e.g. int-to-pointer, int 
      // plus minimum, etc.):
      //
      if (vt.is_pointer()) {
         to_assign = to_assign.conversion_sans_bytecode(vt);
      } else if (o.min != 0) {
         //
         // If a field's range of valid values is [a, b], then serialize it as (v - a), 
         // and then unpack it as (v + a). This means we don't need a sign bit when we 
         // serialize negative numbers, and it also means that ranges that start above 
         // zero don't waste the low bits.
         //
         to_assign = to_assign.add(
            gw::expr::integer_constant(vt, o.min)
         );
      }
      //
      // Create assignment expression.
      //
      return gw::expr::assign(
         dst,
         to_assign
      );
   }
   
   gcc_wrappers::expr::base stream_function_set::make_save_expression_for(
      gcc_wrappers::value state_pointer,
      gcc_wrappers::value item,
      const expression_options& options
   ) const {
      const auto& ty = gw::builtin_types::get_fast();
      
      auto vt = item.value_type();
      assert(!vt.is_record() && !vt.is_union());
      
      // Handle strings.
      if (vt.is_array()) {
         const auto& o = options.string;
         assert(vt.array_value_type() == this->types.string_char);
         assert(o.max_length != std::numeric_limits<size_t>::max() && "Specify the max length!");
         
         gw::decl::function func;
         if (o.needs_terminator) {
            func = this->save.stream_wt;
         } else {
            func = this->save.stream_ut;
         }
         
         return gw::expr::call(
            func,
            // args:
            state_pointer,
            item.convert_array_to_pointer(),
            gw::expr::integer_constant(ty.uint16, o.max_length)
         );
      }
      
      // Handle buffers.
      if (vt.is_pointer()) {
         auto pointed_to = vt.remove_pointer();
         assert(pointed_to.is_record() || pointed_to.is_union());
         
         if (pointed_to != this->types.buffer_byte) {
            item = item.conversion_sans_bytecode(this->types.buffer_byte.add_pointer());
         }
         
         const auto& o = options.buffer;
         return gw::expr::call(
            this->save.buffer,
            // args:
            state_pointer,
            item,
            gw::expr::integer_constant(ty.uint16, o.bytecount)
         );
      }
      
      const auto& o = options.integral;
      assert(o.bitcount != std::numeric_limits<size_t>::max() && "Specify the integral's bitcount!");
      if (vt == this->types.boolean && o.bitcount == 1) {
         return gw::expr::call(
            this->save.boolean,
            // args:
            state_pointer,
            item
         );
      }
      
      gw::decl::function func;
      if (vt.is_pointer()) {
         assert(o.min == 0 && "Cannot support minimum values, offsets, etc., on pointers!");
         //
         // TODO: Support other pointer widths!
         //
         func = this->save.u32;
         item = item.conversion_sans_bytecode(ty.uint32);
      } else {
         auto vt_canonical = vt.canonical();
         if (vt_canonical == ty.uint8) {
            func = this->save.u8;
         } else if (vt_canonical == ty.uint16) {
            func = this->save.u16;
         } else if (vt_canonical == ty.uint32) {
            func = this->save.u32;
         } else if (vt_canonical == ty.int8) {
            func = this->save.s8;
         } else if (vt_canonical == ty.int16) {
            func = this->save.s16;
         } else if (vt_canonical == ty.int32) {
            func = this->save.s32;
         }
         
         if (o.min != 0) {
            //
            // If a field's range of valid values is [a, b], then serialize it as (v - a), 
            // and then unpack it as (v + a). This means we don't need a sign bit when we 
            // serialize negative numbers, and it also means that ranges that start above 
            // zero don't waste the low bits.
            //
            item = item.sub(gw::expr::integer_constant(vt, o.min));
         }
      }
      assert(!func.empty());
      
      return gw::expr::call(
         func,
         // args:
         state_pointer,
         item,
         gw::expr::integer_constant(ty.uint8, o.bitcount)
      );
   }
}