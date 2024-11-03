#include "bitpacking/global_options.h"
#include <stdexcept>

#include <stringpool.h> // get_identifier
#include <c-family/c-common.h> // lookup_name

#include "lu/strings/printf_string.h"
#include "gcc_wrappers/builtin_types.h"

namespace bitpacking::global_options {
   void requested::consume_pragma_kv_set(const gcc_helpers::pragma_kv_set& set) {
      auto _pull_size = [&set](const char* key, const char* noun, size_t& dst) {
         auto it = set.find(key);
         if (it == set.end())
            return;
         auto& variant = it->second;
         if (!std::holds_alternative<intmax_t>(variant)) {
            std::string message = "the ";
            message += noun;
            message += " must be specified as an integer literal";
            throw std::runtime_error(message);
         }
         auto value = std::get<intmax_t>(variant);
         if (value < 0) {
            std::string message = "the ";
            message += noun;
            message += " must not be zero or negative";
            throw std::runtime_error(message);
         }
         dst = value;
      };
      
      _pull_size(
         "sector_count",
         "bitpack sector count",
         this->sectors.max_count
      );
      _pull_size(
         "sector_size",
         "bitpack sector size",
         this->sectors.size_per
      );
      
      auto _pull_identifier = [&set](std::string key, std::string noun, std::string& dst) {
         auto it = set.find(key);
         if (it == set.end())
            return;
         auto& variant = it->second;
         if (variant.index() != 1) {
            std::string message = "the ";
            message += noun;
            message += " must be specified as an identifier";
            throw std::runtime_error(message);
         }
         dst = std::get<1>(variant);
      };
      
      auto _pull_identifier_set = [&_pull_identifier](
         const char* key_frag,
         const char* noun_frag,
         function_set& dst
      ) {
         auto _name = [key_frag](const char* a, const char* b) -> std::string {
            return std::string(a) + key_frag + b;
         };
         
         _pull_identifier(
            _name("func_", "_bool"),
            _name("bitpacking ", " function for bool values"),
            dst.boolean
         );
         _pull_identifier(
            _name("func_", "_s8"),
            _name("bitpacking ", " function for int8_t values"),
            dst.s8
         );
         _pull_identifier(
            _name("func_", "_s16"),
            _name("bitpacking ", " function for int16_t values"),
            dst.s16
         );
         _pull_identifier(
            _name("func_", "_s32"),
            _name("bitpacking ", " function for int32t values"),
            dst.s16
         );
         _pull_identifier(
            _name("func_", "_u8"),
            _name("bitpacking ", " function for uint8_t values"),
            dst.u8
         );
         _pull_identifier(
            _name("func_", "_u16"),
            _name("bitpacking ", " function for uint16_t values"),
            dst.u16
         );
         _pull_identifier(
            _name("func_", "_u32"),
            _name("bitpacking ", " function for uint32_t values"),
            dst.u16
         );
         _pull_identifier(
            _name("func_", "_buffer"),
            _name("bitpacking ", " function for arbitrary buffers"),
            dst.buffer
         );
         _pull_identifier(
            _name("func_", "_string"),
            _name("bitpacking ", " function for strings with optional terminators"),
            dst.string_ut
         );
         _pull_identifier(
            _name("func_", "_string_terminated"),
            _name("bitpacking ", " function for strings with terminators"),
            dst.string_wt
         );
      };
      //
      _pull_identifier_set("read",  "read",  this->functions.read);
      _pull_identifier_set("write", "write", this->functions.write);
      
      _pull_identifier("func_initialize", "bitstream initialize function", this->functions.stream_state_init);
      
      _pull_identifier("bool_typename", "bool typename", this->types.boolean);
      _pull_identifier("buffer_byte_typename", "buffer byte typename", this->types.buffer_byte);
      _pull_identifier("string_char_typename", "buffer byte typename", this->types.string_char);
      _pull_identifier("bitstream_state_typename", "bitstream state typename", this->types.bitstream_state);
   }
}

namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
   
   namespace {
      gw::decl::function _get_func_decl(const char* identifier) {
         auto node = lookup_name(get_identifier(identifier));
         if (!gw::decl::function::node_is(node))
            node = NULL_TREE;
         return gw::decl::function::from_untyped(node);
      }
      gw::decl::function _get_func_decl(const std::string& identifier) {
         return _get_func_decl(identifier.c_str());
      }
      gw::type _get_type_decl(const char* identifier) {
         auto node = lookup_name(get_identifier(identifier));
         if (node != NULL_TREE) {
            if (TREE_CODE(node) == TYPE_DECL) {
               node = TREE_TYPE(node);
            } else if (!gw::type::node_is(node)) {
               node = NULL_TREE;
            }
         }
         return gw::type::from_untyped(node);
      }
   }
   
   // "Check" functions for types.
   namespace {
      void _check_exists(const char* noun, gw::type type) {
         if (!type.empty())
            return;
         throw std::runtime_error(lu::strings::printf_string(
            "the %<%s%> type is missing",
            noun
         ));
      }
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
            "the %<%s%> function is missing",
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
            func_decl.name().data()
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
         auto arg_type = gw::type::from_untyped((*arg_it).second);
         if (arg_type.empty()) {
            throw std::runtime_error(lu::strings::printf_string(
               "too few arguments for the %%<%s%%> function (%%<%s%%>) (only %u present)",
               noun,
               func_decl.name().data(),
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
            func_decl.name().data(),
            desired_arg_type.pretty_print().c_str(),
            arg_type.pretty_print().c_str()
         ));
      }
      
      void _check_argument_endcap(
         const char*             noun,
         gw::decl::function      func_decl,
         gw::list_node::iterator arg_it
      ) {
         const auto& ty = gw::builtin_types::get_fast();
         
         auto raw_arg_type = (*arg_it).second;
         if (raw_arg_type == ty.basic_void.as_untyped())
            return;
         if (raw_arg_type == NULL_TREE) {
            auto arg_type = gw::type::from_untyped(raw_arg_type);
            throw std::runtime_error(lu::strings::printf_string(
               "extra argument(s) found for the %%<%s%%> function (%%<%s%%>) (first extra argument type is %%<%s%%>)",
               noun,
               func_decl.name().data(),
               arg_type.pretty_print().c_str()
            ));
         }
         throw std::runtime_error(lu::strings::printf_string(
            "the %%<%s%%> function (%%<%s%%>) is varargs; this is wrong",
            noun,
            func_decl.name().data()
         ));
      }
   }
}

namespace bitpacking::global_options {
   void computed::resolve(const requested& src) {
      const auto& ty = gw::builtin_types::get_fast();
      
      //
      // Resolve types:
      //
      
      this->types.boolean     = ty.basic_bool;
      this->types.string_char = ty.basic_char;
      if (!src.types.boolean.empty()) {
         if (src.types.boolean != "bool") {
            this->types.boolean = _get_type_decl(src.types.boolean.c_str());
         }
      }
      if (!src.types.string_char.empty()) {
         if (src.types.string_char != "char") {
            this->types.string_char = _get_type_decl(src.types.string_char.c_str());
         }
      }
      this->types.bitstream_state = _get_type_decl(src.types.bitstream_state.c_str());
      this->types.buffer_byte     = _get_type_decl(src.types.buffer_byte.c_str());
      
      _check_exists("boolean", this->types.boolean);
      _check_exists("string char", this->types.string_char);
      _check_exists("bitstream state", this->types.bitstream_state);
      _check_exists("buffer byte", this->types.buffer_byte);
      
      this->types.buffer_byte_ptr = this->types.buffer_byte.add_pointer();
      auto stream_state_ptr_type = this->types.bitstream_state_ptr = this->types.bitstream_state.add_pointer();
      
      //
      // Resolve functions:
      //
      
      // void lu_BitstreamInitialize(struct lu_BitstreamState*, uint8_t* buffer)
      {
         constexpr const char* noun = "bitstream initialize function";
         
         this->functions.stream_state_init = _get_func_decl(src.functions.stream_state_init);
         auto decl = this->functions.stream_state_init;
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
         auto& set = this->functions.read;
         
         // bool lu_BitstreamRead_bool(struct lu_BitstreamState*)
         {
            auto noun_str = lu::strings::printf_string("bitstream %s function for bool values", op_name);
            auto noun = noun_str.c_str();
            
            set.boolean = _get_func_decl(src.functions.read.boolean);
            auto decl = set.boolean;
            _check_exists(noun, decl);
            auto type = decl.function_type();
            _check_prototyped  (noun, decl, type);
            _check_return_type (noun, decl, type, this->types.boolean);
            
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
            auto noun_str = lu::strings::printf_string("bitstream %s function for %s values", op_name, noun_type);
            auto noun = noun_str.c_str();
            
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
         
         set.u8  = _get_func_decl(src.functions.read.u8);
         set.u16 = _get_func_decl(src.functions.read.u16);
         set.u32 = _get_func_decl(src.functions.read.u32);
         //
         set.s8  = _get_func_decl(src.functions.read.s8);
         set.s16 = _get_func_decl(src.functions.read.s16);
         set.s32 = _get_func_decl(src.functions.read.s32);
         
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
            auto noun_str = lu::strings::printf_string("bitstream %s function for %s", op_name, noun_type);
            auto noun = noun_str.c_str();
            
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
         
         set.string_ut = _get_func_decl(src.functions.read.string_ut);
         set.string_wt = _get_func_decl(src.functions.read.string_wt);
         set.buffer    = _get_func_decl(src.functions.read.buffer);
         
         _check_span("strings with terminators", set.string_wt, this->types.string_char);
         _check_span("strings with optional terminators", set.string_ut, this->types.string_char);
         _check_span("buffers", set.buffer, this->types.buffer_byte);
      }
      
      // bitstream write
      {
         constexpr const char* op_name = "write";
         auto& set = this->functions.save;
         
         // void lu_BitstreamRead_bool(struct lu_BitstreamState*, bool)
         {
            auto noun_str = lu::strings::printf_string("bitstream %s function for bool values", op_name);
            auto noun = noun_str.c_str();
            
            auto decl = set.boolean;
            _check_exists(noun, decl);
            auto type = decl.function_type();
            _check_prototyped  (noun, decl, type);
            _check_return_type (noun, decl, type, ty.basic_void);
            
            auto   it = type.function_arguments().begin();
            size_t i  = 0;
            _check_nth_argument    (noun, decl,   i,   it, stream_state_ptr_type);
            _check_nth_argument    (noun, decl,   i,   it, this->types.boolean);
            _check_argument_endcap (noun, decl, ++it);
         }
         
         // void lu_BitstreamWrite_T(struct lu_BitstreamState*, T, uint8_t bitcount)
         auto _check_integer = [op_name, stream_state_ptr_type, &ty](
            const char*        noun_type,
            gw::decl::function decl,
            gw::type           result_type
         ) {
            auto noun_str = lu::strings::printf_string("bitstream %s function for %s values", op_name, noun_type);
            auto noun = noun_str.c_str();
            
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
         
         set.u8  = _get_func_decl(src.functions.write.u8);
         set.u16 = _get_func_decl(src.functions.write.u16);
         set.u32 = _get_func_decl(src.functions.write.u32);
         //
         set.s8  = _get_func_decl(src.functions.write.s8);
         set.s16 = _get_func_decl(src.functions.write.s16);
         set.s32 = _get_func_decl(src.functions.write.s32);
         
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
            auto noun_str = lu::strings::printf_string("bitstream %s function for %s", op_name, noun_type);
            auto noun = noun_str.c_str();
            
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
         
         set.string_ut = _get_func_decl(src.functions.write.string_ut);
         set.string_wt = _get_func_decl(src.functions.write.string_wt);
         set.buffer    = _get_func_decl(src.functions.write.buffer);
         
         _check_span("strings with terminators", set.string_wt, this->types.string_char);
         _check_span("strings with optional terminators", set.string_ut, this->types.string_char);
         _check_span("buffers", set.buffer, this->types.buffer_byte);
      }
      
      // Done.
   }
}