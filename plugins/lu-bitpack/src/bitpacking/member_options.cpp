#include "bitpacking/member_options.h"
#include <cassert>
#include "bitpacking/heritable_options.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include <diagnostic.h> // warning(), error(), etc.

namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
   
   static bool type_is_string_or_array_thereof(
      const bitpacking::global_options::computed& global,
      gw::type type
   ) {
      auto at = type;
      auto et = at.array_value_type();
      do {
         if (!et.is_array()) {
            if (et == global.types.string_char) {
               return true;
            }
            break;
         }
         at = et;
         et = et.array_value_type();
      } while (true);
      return false;
   }
}

namespace bitpacking::member_options {
   void requested::from_field(gcc_wrappers::decl::field decl) {
      auto name = decl.name();
      auto list = decl.attributes();
      auto loc  = decl.source_location();
      
      auto _report_error = [&decl]<typename... Args>(const char* fmt, Args&&... args) {
         std::string message = lu::strings::printf_string(
            "invalid bitpacking options for field %%<%s%%>: ",
            decl.name().data()
         );
         message += lu::strings::printf_string(fmt, args...);
         
         error_at(decl.source_location, message.c_str());
      };
      
      for(const auto pair : list) {
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
            auto opt = gw::expr::integer_constant::from_untyped(pair.second).value_signed();
            if (!opt.has_value()) {
               _report_error(
                  "\"lu_bitpack_bitcount\" attribute must have an integer constant parameter "
                  "indicating the number of bits to pack the field into."
               );
               continue;
            }
            auto v = *opt;
            if (v <= 0) {
               _report_error(
                  "\"lu_bitpack_bitcount\" attribute must specifyh a positive non-zero "
                  "bitcount (seen: %d)",
                  v
               );
               continue;
            }
            if (v > sizeof(size_t) * 8) {
               warning_at(
                  decl.source_location(),
                  1,
                  "unusual bitpacking options for field %s: \"lu_bitpack_bitcount\" attribute "
                  "specifies an unusually large bitcount; is this intentional? (seen: %d)",
                  decl.name().data(),
                  v
               );
            }
            this->integral.bitcount = v;
            continue;
         }
         if (key == "lu_bitpack_funcs") {
            std::string pre_pack;
            std::string post_unpack;
            if (TREE_CODE(pair.second) == STRING_CST) {
               auto data = gw::expr::string_constant::from_untyped(pair.second).value();
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
                     "\"lu_bitpack_funcs\" attribute failed to parse: %s",
                     ex.what()
                  );
                  continue;
               }
            }
            this->transform.pre_pack = pre_pack;
            this->transform.post_unpack = post_unpack;
            continue;
         }
         if (key == "lu_bitpack_inherit") {
            std::string from;
            if (TREE_CODE(pair.second) == STRING_CST) {
               auto data = gw::expr::string_constant::from_untyped(pair.second).value();
               static_assert(false, "TODO: parse");
            }
            this->inherit_from = from;
            continue;
         }
         if (key == "lu_bitpack_range") {
            list_node args(pair.second);
            if (args.empty()) {
               _report_error(
                  "\"lu_bitpack_range\" attribute must specify two arguments (the minimum "
                  "and maximum possible values of the field); no arguments given"
               );
               continue;
            } else {
               auto size = args.size();
               if (size < 2) {
                  _report_error(
                     "\"lu_bitpack_range\" attribute must specify two arguments (the minimum "
                     "and maximum possible values of the field); only one argument given"
                  );
                  continue;
               } else if (size > 2) {
                  warning_at(
                     decl.source_location(),
                     1,
                     "unusual bitpacking options for field %s: \"lu_bitpack_range\" attribute "
                     "specifies more (%u) arguments than expected (2)",
                     (int)size
                  );
               }
            }
            
            auto arg_a = args[0];
            auto arg_b = args[1];
            {
               auto opt = gw::expr::integer_constant::from_untyped(arg_a).value_signed();
               if (!opt.has_value()) {
                  _report_error(
                     "\"lu_bitpack_range\" first argument doesn't appear to be an integer "
                     "constant"
                  );
                  continue;
               }
               this->integral.min = *opt;
            }
            {
               auto opt = gw::expr::integer_constant::from_untyped(arg_b).value_signed();
               if (!opt.has_value()) {
                  _report_error(
                     "\"lu_bitpack_range\" second argument doesn't appear to be an integer "
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
                  "\"lu_bitpack_string\" used on a field that isn't a string (array of char)"
               );
               continue;
            }
            //
            // TODO: Validate that the array is of the right type? Requires having access to 
            // the global bitpacking options from here.
            //
            this->is_explicit_string = true;
            if (TREE_CODE(pair.second) == STRING_CST) {
               auto data = gw::expr::string_constant::from_untyped(pair.second).value();
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
                           "\"lu_bitpack_string\" attribute must specify a positive non-zero "
                           "string length (seen: %d)",
                           v
                        );
                        continue;
                     }
                     this->string.length = v;
                  }
                  this->string.null_terminator = null_terminated;
               } catch (std::runtime_error& ex) {
                  _report_error(
                     "\"lu_bitpack_string\" attribute failed to parse: %s",
                     ex.what()
                  );
                  continue;
               }
            }
            continue;
         }
         
         // Done.
      }
   }
   
   void computed::resolve(
      const global_options::computed& global,
      const requested& src,
      gw::type member_type
   ) {
      assert(!src.omit_from_bitpacking);
      
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
         if (!type_is_string_or_array_thereof(global, member_type)) {
            throw std::runtime_error("string bitpacking options applied to a data member that is not a string or array thereof");
         }
         
         this->kind = member_kind::string;
         auto& dst = this->data.emplace<string_data>();
         {
            auto opt = at.array_extent();
            if (!opt.has_value()) {
               throw std::runtime_error("string bitpacking options applied to a variable-length array; VLAs are not supported");
            }
            dst.length = *opt;
            
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
         if (src.string.with_terminator.has_value()) {
            dst.with_terminator = *src.string.with_terminator;
         } else {
            if (inherit_from && is_string_h) {
               auto opt = std::get<heritable_options::string_data>(inherit_from->data).with_terminator;
               if (opt.has_value())
                  dst.with_terminator = *opt;
            }
         }
         return;
      }
      
      //
      // Recurse through arrays.
      //
      auto array_type = member_type;
      auto value_type = array_type.array_value_type();
      do {
         if (!value_type.is_array()) {
            break;
         }
         array_type = value_type;
         value_type = array_type.array_value_type();
      } while (true);
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
      
      this->kind = member_kind::integral;
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
         auto integral_info = this->type.get_integral_info();
         
         std::intmax_t  min;
         std::uintmax_t max;
         if (opt_min.has_value()) {
            min = *opt_min;
         } else {
            min = integral_info.min;
         }
         if (opt_max.has_value()) {
            max = *opt_max;
         } else {
            max = integral_info.max;
         }
         
         dst.min = min;
         dst.bitcount = std::bit_width((std::uintmax_t)(max - min));
      }
      return;
   }
}