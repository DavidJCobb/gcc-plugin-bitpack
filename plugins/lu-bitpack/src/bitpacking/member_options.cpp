#include "bitpacking/member_options.h"
#include <bit>
#include <cassert>
#include <stdexcept>
#include "lu/strings/handle_kv_string.h"
#include "lu/strings/printf_string.h"
#include "bitpacking/global_options.h"
#include "bitpacking/heritable_options.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/type/array.h"
#include <diagnostic.h> // warning(), error(), etc.

namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
   
   static bool type_is_string_or_array_thereof(
      const bitpacking::global_options::computed& global,
      gw::type::array type
   ) {
      gw::type::array at = type;
      gw::type::base  et = at.value_type();
      do {
         if (!et.is_array()) {
            if (et == global.types.string_char) {
               return true;
            }
            break;
         }
         at = et.as_array();
         et = at.value_type();
      } while (true);
      return false;
   }
   
   static std::optional<size_t> string_length(
      const bitpacking::global_options::computed& global,
      gw::type::array type
   ) {
      auto at = type;
      auto et = at.value_type();
      do {
         if (!et.is_array()) {
            if (et == global.types.string_char) {
               return at.extent();
            }
            break;
         }
         at = et.as_array();
         et = at.value_type();
      } while (true);
      return false;
   }
}

namespace bitpacking::member_options {
   void requested::from_field(gcc_wrappers::decl::field decl) {
      auto list = decl.attributes();
      
      auto _report_error = [&decl]<typename... Args>(const char* fmt, Args&&... args) {
         std::string message = lu::strings::printf_string(
            "invalid bitpacking options for field %%<%s%%>: ",
            decl.name().data()
         );
         message += lu::strings::printf_string(fmt, args...);
         
         error_at(decl.source_location(), message.c_str());
      };
      
      for(const auto pair : list) {
         auto args = gw::list_node(pair.second);
         
         std::string_view key;
         {
            if (pair.first == NULL_TREE)
               continue;
            if (TREE_CODE(pair.first) != IDENTIFIER_NODE)
               continue;
            key = IDENTIFIER_POINTER(pair.first);
         }
         
         if (key == "lu_bitpack_omit") {
            this->omit_from_bitpacking = true;
            continue;
         }
         if (key == "lu_bitpack_as_opaque_buffer") {
            this->is_explicit_buffer = true;
            continue;
         }
         if (key == "lu_bitpack_bitcount") {
            int v;
            {
               auto arg = args.untyped_nth_value(0);
               if (arg == NULL_TREE || !gw::expr::integer_constant::node_is(arg)) {
                  _report_error(
                     "%<lu_bitpack_bitcount%> attribute must have an integer constant parameter "
                     "indicating the number of bits to pack the field into."
                  );
                  continue;
               }
               auto opt = gw::expr::integer_constant::from_untyped(arg).try_value_signed();
               if (!opt.has_value()) {
                  _report_error(
                     "%<lu_bitpack_bitcount%> attribute must have an integer constant parameter "
                     "indicating the number of bits to pack the field into."
                  );
                  continue;
               }
               v = (int) *opt;
            }
            if (v <= 0) {
               _report_error(
                  "%<lu_bitpack_bitcount%> attribute must specifyh a positive non-zero "
                  "bitcount (seen: %d)",
                  (int)v
               );
               continue;
            }
            if (v > (int)sizeof(size_t) * 8) {
               warning_at(
                  decl.source_location(),
                  1,
                  "unusual bitpacking options for field %<%s%>: %<lu_bitpack_bitcount%> attribute "
                  "specifies an unusually large bitcount; is this intentional? (seen: %<%d%>)",
                  decl.name().data(),
                  (int)v
               );
            }
            this->integral.bitcount = v;
            continue;
         }
         if (key == "lu_bitpack_funcs") {
            std::string pre_pack;
            std::string post_unpack;
            
            if (!args.empty()) {
               auto arg = args.untyped_nth_value(0);
               if (TREE_CODE(arg) != STRING_CST) {
                  _report_error(
                     "%<lu_bitpack_funcs%> attribute argument must be a string literal of "
                     "the form \"pre_pack=identifier,post_unpack=identifier\""
                  );
                  continue;
               }
               auto data = gw::expr::string_constant::from_untyped(arg).value();
               try {
                  lu::strings::handle_kv_string(data, std::array{
                     lu::strings::kv_string_param{
                        .name      = "pre_pack",
                        .has_param = true,
                        .handler   = [&pre_pack](std::string_view v, int vi) {
                           pre_pack = v;
                        },
                     },
                     lu::strings::kv_string_param{
                        .name      = "post_unpack",
                        .has_param = false,
                        .handler   = [&post_unpack](std::string_view v, int vi) {
                           post_unpack = v;
                        },
                     },
                  });
               } catch (std::runtime_error& ex) {
                  _report_error(
                     "%<lu_bitpack_funcs%> attribute failed to parse: %s",
                     ex.what()
                  );
                  continue;
               }
            }
            this->transforms.pre_pack = pre_pack;
            this->transforms.post_unpack = post_unpack;
            continue;
         }
         if (key == "lu_bitpack_inherit") {
            if (args.empty()) {
               _report_error("%<lu_bitpack_inherit%> attribute: string literal argument required");
               continue;
            }
            auto arg = args.untyped_nth_value(0);
            if (TREE_CODE(arg) != STRING_CST) {
               _report_error("%<lu_bitpack_inherit%> attribute: string literal argument required");
               continue;
            }
            this->inherit_from = gw::expr::string_constant::from_untyped(arg).value();
            continue;
         }
         if (key == "lu_bitpack_range") {
            if (args.empty()) {
               _report_error(
                  "%<lu_bitpack_range%> attribute must specify two arguments (the minimum "
                  "and maximum possible values of the field); no arguments given"
               );
               continue;
            } else {
               auto size = args.size();
               if (size < 2) {
                  _report_error(
                     "%<lu_bitpack_range%> attribute must specify two arguments (the minimum "
                     "and maximum possible values of the field); only one argument given"
                  );
                  continue;
               } else if (size > 2) {
                  warning_at(
                     decl.source_location(),
                     1,
                     "unusual bitpacking options for field %s: %<lu_bitpack_range%> attribute "
                     "specifies more (%u) arguments than expected (2)",
                     decl.name().data(),
                     (int)size
                  );
               }
            }
            
            auto arg_a = args.untyped_nth_value(0);
            auto arg_b = args.untyped_nth_value(1);
            {
               auto opt = gw::expr::integer_constant::from_untyped(arg_a).try_value_signed();
               if (!opt.has_value()) {
                  _report_error(
                     "%<lu_bitpack_range%> first argument doesn't appear to be an integer "
                     "constant"
                  );
                  continue;
               }
               this->integral.min = *opt;
            }
            {
               auto opt = gw::expr::integer_constant::from_untyped(arg_b).try_value_signed();
               if (!opt.has_value()) {
                  _report_error(
                     "%<lu_bitpack_range%> second argument doesn't appear to be an integer "
                     "constant"
                  );
                  continue;
               }
               this->integral.max = *opt;
            }
            continue;
         }
         if (key == "lu_bitpack_string") {
            auto type = decl.value_type();
            if (!type.is_array()) {
               _report_error(
                  "%<lu_bitpack_string%> used on a field that isn't a string (array of char)"
               );
               continue;
            }
            //
            // TODO: Validate that the array is of the right type? Requires having access to 
            // the global bitpacking options from here.
            //
            this->is_explicit_string = true;
            if (!args.empty()) {
               auto arg = args.untyped_nth_value(0);
               if (arg == NULL_TREE || TREE_CODE(arg) != STRING_CST) {
                  _report_error("%<lu_bitpack_string%> attribute argument must be a string literal");
                  continue;
               }
               auto data = gw::expr::string_constant::from_untyped(arg).value();
               try {
                  std::optional<int> length;
                  bool null_terminated = false;
                  
                  lu::strings::handle_kv_string(data, std::array{
                     lu::strings::kv_string_param{
                        .name      = "length",
                        .has_param = true,
                        .int_param = true,
                        .handler   = [&length](std::string_view v, int vi) {
                           length = vi;
                        },
                     },
                     lu::strings::kv_string_param{
                        .name      = "with-terminator",
                        .has_param = false,
                        .handler   = [&null_terminated](std::string_view v, int vi) {
                           null_terminated = true;
                        },
                     },
                  });
                  if (length.has_value()) {
                     auto v = *length;
                     if (v <= 0) {
                        _report_error(
                           "%<lu_bitpack_string%> attribute must specify a positive non-zero "
                           "string length (seen: %d)",
                           (int)v
                        );
                        continue;
                     }
                     this->string.length = v;
                  }
                  this->string.with_terminator = null_terminated;
               } catch (std::runtime_error& ex) {
                  _report_error(
                     "%<lu_bitpack_string%> attribute failed to parse: %s",
                     ex.what()
                  );
                  continue;
               }
            }
            continue;
         }
         
         if (key.starts_with("lu_bitpack_")) {
            warning_at(
               decl.source_location(),
               1,
               "unrecognized bitpacking attribute %<%s%> on field %<%s%>",
               key.data(),
               decl.name().data()
            );
         }
         
         // Done.
      }
   }
   
   void computed::resolve(
      const global_options::computed& global,
      const requested& src,
      gw::type::base member_type
   ) {
      assert(!src.omit_from_bitpacking);
      
      this->transforms.pre_pack    = src.transforms.pre_pack;
      this->transforms.post_unpack = src.transforms.post_unpack;
      
      const heritable_options* inherit_from = nullptr;
      bool is_integer_h = false;
      bool is_string_h  = false;
      if (!src.inherit_from.empty()) {
         inherit_from = heritable_options_stockpile::get().options_by_name(src.inherit_from);
         if (inherit_from == nullptr) {
            throw std::runtime_error(lu::strings::printf_string("data member inherits from undefined heritable option set %s", src.inherit_from.c_str()));
         }
         is_integer_h = std::holds_alternative<heritable_options::integral_data>(inherit_from->data);
         is_string_h  = std::holds_alternative<heritable_options::string_data>(inherit_from->data);
      }
      
      bool is_string = src.is_explicit_string;
      if (inherit_from) {
         if (is_string_h) {
            is_string = true;
         } else {
            if (src.is_explicit_string) {
               throw std::runtime_error("contradictory bitpacking options (this field is a string, but inherits an option set of another type)");
            }
         }
      }
      
      if (is_string) {
         if (src.is_explicit_buffer) {
            if (is_string_h) {
               throw std::runtime_error("contradictory bitpacking options (this field is an opaque buffer, but inherits an option set of another type)");
            }
            throw std::runtime_error("contradictory bitpacking options (opaque buffer AND string)");
         }
         if (
            !member_type.is_array() ||
            !type_is_string_or_array_thereof(global, member_type.as_array())
         ) {
            throw std::runtime_error("string bitpacking options applied to a data member that is not a string or array thereof");
         }
         
         if (
            src.integral.bitcount.has_value() || 
            src.integral.min.has_value() || 
            src.integral.max.has_value()
         ) {
            throw std::runtime_error("contradictory bitpacking options (this field uses integral bitpacking options, but inherits a string option set)");
         }
         
         this->kind = member_kind::string;
         auto& dst = this->data.emplace<string_data>();
         if (src.string.with_terminator.has_value()) {
            dst.with_terminator = *src.string.with_terminator;
         } else {
            if (inherit_from && is_string_h) {
               auto opt = std::get<heritable_options::string_data>(inherit_from->data).with_terminator;
               if (opt.has_value())
                  dst.with_terminator = *opt;
            }
         }
         {
            auto opt = string_length(global, member_type.as_array());
            if (!opt.has_value()) {
               throw std::runtime_error("string bitpacking options applied to a variable-length array; VLAs are not supported");
            }
            dst.length = *opt;
            if (dst.length == 0) {
               throw std::runtime_error("string bitpacking options applied to a zero-length string");
            }
            if (dst.with_terminator) {
               --dst.length;
               if (dst.length == 0) {
                  throw std::runtime_error("string bitpacking options applied to a zero-length string (array of one character, to be burned on a null terminator)");
               }
            }
            
            std::optional<size_t> override_length;
            if (inherit_from) {
               override_length = std::get<heritable_options::string_data>(inherit_from->data).length;
            }
            if (src.string.length.has_value()) {
               override_length = src.string.length;
            }
            
            if (override_length.has_value()) {
               auto l = *override_length;
               if (l > dst.length) {
                  throw std::runtime_error(lu::strings::printf_string(
                     "the specified length (%u) is too large to fit in the target data member (array extent %u)",
                     l,
                     dst.length
                  ));
               }
               dst.length = l;
            }
         }
         return;
      }
      
      //
      // Recurse through arrays.
      //
      auto value_type = member_type;
      if (member_type.is_array()) {
         auto array_type = member_type.as_array();
         value_type = array_type.value_type();
         do {
            if (!value_type.is_array()) {
               break;
            }
            array_type = value_type.as_array();
            value_type = array_type.value_type();
         } while (true);
      }
      if (value_type.empty()) {
         throw std::runtime_error("failed to access member type??");
      }
      
      if (src.is_explicit_buffer) {
         if (inherit_from) {
            throw std::runtime_error("contradictory bitpacking options (this field is an opaque buffer, but inherits an option set of another type)");
         }
         this->kind = member_kind::buffer;
         this->data = buffer_data{
            .bytecount = value_type.size_in_bytes(),
         };
         return;
      }
      
      if (member_type.is_record()) {
         this->kind = member_kind::structure;
         
         //
         // TODO: pull transform options from the attributes on `member_type`, if 
         //       they're present.
         //
         
         return;
      }
      
      //
      // Assume integral.
      //
      
      if (value_type.is_pointer()) {
         if (src.integral.min.has_value() || src.integral.max.has_value()) {
            throw std::runtime_error("specifying a minimum or maximum integer value for pointers is invalid");
         }
         if (src.integral.bitcount.has_value() && *src.integral.bitcount != value_type.size_in_bits()) {
            throw std::runtime_error("overriding the bitcount for pointers is invalid");
         }
         if (is_integer_h) {
            const auto& data = std::get<heritable_options::integral_data>(inherit_from->data);
            if (data.min.has_value() || data.max.has_value()) {
               throw std::runtime_error("specifying a minimum or maximum integer value for pointers is invalid (pointer field inherits from an integer option set that sets these options)");
            }
            if (data.bitcount.has_value() && *src.integral.bitcount != value_type.size_in_bits()) {
               throw std::runtime_error("overriding the bitcount for pointers is invalid (pointer field inherits from an integer option set that sets these options)");
            }
         }
         this->kind = member_kind::pointer;
         this->data = integral_data{
            .bitcount = value_type.size_in_bits(),
            .min      = 0
         };
         return;
      }
      
      if (
         !value_type.is_boolean() &&
         !value_type.is_enum()    &&
         !value_type.is_integer() &&
         !value_type.is_pointer()
      ) {
         throw std::runtime_error("unsupported member type detected while computing final bitpacking options");
      }
      
      this->kind = member_kind::integer;
      auto& dst = this->data.emplace<integral_data>();
      
      std::optional<size_t>    opt_bitcount;
      std::optional<intmax_t>  opt_min;
      std::optional<uintmax_t> opt_max;
      if (is_integer_h) {
         const auto& data = std::get<heritable_options::integral_data>(inherit_from->data);
         opt_bitcount = data.bitcount;
         opt_min      = data.min;
         opt_max      = data.max;
      }
      if (auto& src_opt = src.integral.bitcount; src_opt.has_value())
         opt_bitcount = src_opt;
      if (auto& src_opt = src.integral.min; src_opt.has_value())
         opt_min = src_opt;
      if (auto& src_opt = src.integral.max; src_opt.has_value())
         opt_max = src_opt;
      
      if (opt_bitcount.has_value()) {
         dst.bitcount = *opt_bitcount;
         if (opt_min.has_value()) {
            dst.min = *opt_min;
         }
         return;
      } else {
         if (value_type.is_type_or_transitive_typedef_thereof(global.types.boolean)) {
            if (!opt_min.has_value() && !opt_max.has_value()) {
               this->kind = member_kind::boolean;
               dst.min      = 0;
               dst.bitcount = 1;
               return;
            }
         }
         
         std::intmax_t  min;
         std::uintmax_t max;
         if (opt_min.has_value()) {
            min = *opt_min;
         } else {
            min = value_type.as_integral().minimum_value();
         }
         if (opt_max.has_value()) {
            max = *opt_max;
         } else {
            max = value_type.as_integral().maximum_value();
         }
         
         dst.min = min;
         dst.bitcount = std::bit_width((std::uintmax_t)(max - min));
      }
      return;
   }
}