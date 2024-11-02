#include "data_member.h"
#include <string_view>
#include <cp/cp-tree.h> // same_type_p
#include <diagnostic.h> // warning(), error(), etc. (transitively included from diagnostic-core.h)
#include "gcc_helpers/array_extent.h"
#include "gcc_helpers/extract_integer_constant_from_tree.h"
#include "gcc_helpers/extract_string_constant_from_tree.h"
#include "gcc_helpers/extract_list_tree_items.h"
#include "gcc_helpers/for_each_in_list_tree.h"
#include "gcc_helpers/type_is_char.h"
#include "gcc_helpers/type_min_max.h"
#include "gcc_helpers/type_name.h"
#include "handle_kv_string.h"

/*static*/ std::optional<DataMember> DataMember::from_tree(tree decl) {
   switch (TREE_CODE(decl)) {
      case FIELD_DECL:
         break;
      case METHOD_TYPE:
      default:
         return {};
   }
   DataMember dst;
   if (TYPE_READONLY(decl)) {
      dst.type.is_const = true;
   }
   {
      auto d_name = DECL_NAME(decl);
      if (d_name) {
         dst.name = IDENTIFIER_POINTER(d_name);
      } else {
         //
         // Anonymous struct?
         //
         dst.name = "<anonymous>";
      }
   }
   
   auto decl_type = TREE_TYPE(decl);
   if (DECL_BIT_FIELD(decl)) {
      dst.type.bitfield_bitcount = gcc_helpers::extract_integer_constant_from_tree<size_t>(DECL_SIZE(decl));
      
      decl_type = DECL_BIT_FIELD_TYPE(decl);
   }
   while (TREE_CODE(decl_type) == ARRAY_TYPE) {
      int extent = 0;
      {
         auto opt = gcc_helpers::array_extent(decl_type);
         if (opt.has_value()) {
            extent = opt.value();
         } else {
            dst.type.is_flexible_array = true;
         }
      }
      dst.type.array_indices.push_back(extent);
      //
      // Get value type.
      //
      decl_type = TREE_TYPE(decl_type);
   }
   if (TREE_CODE(decl_type) == REFERENCE_TYPE) {
      dst.type.name        = gcc_helpers::type_name(TREE_TYPE(decl_type));
      dst.type.fundamental = fundamental_type::reference;
      return dst;
   }
   
   switch (TREE_CODE(decl_type)) {
      default:
         dst.type.name        = gcc_helpers::type_name(decl_type);
         dst.type.fundamental = fundamental_type::unknown;
         break;
      case ENUMERAL_TYPE:
         dst.type.name        = gcc_helpers::type_name(decl_type);
         dst.type.fundamental = fundamental_type::enumeration;
         {
            auto pair = gcc_helpers::type_min_max(decl_type);
            dst.type.enum_bounds.min = pair.first.value_or(std::numeric_limits<int64_t>::lowest());
            dst.type.enum_bounds.max = pair.second.value_or(std::numeric_limits<uint64_t>::max());
         }
         break;
      case INTEGER_TYPE:
         dst.type.name        = gcc_helpers::type_name(decl_type);
         dst.type.fundamental = fundamental_type::integer;
         {
            auto pair = gcc_helpers::type_min_max(decl_type);
            if (pair.first.has_value() && pair.second.has_value()) {
               const auto min = pair.first.value();
               const auto max = pair.second.value();
               for(auto* known : known_integrals::all) {
                  if (min == known->min && max == known->max) {
                     dst.type.integral = known;
                     break;
                  }
               }
            }
         }
         break;
      case RECORD_TYPE:
      case UNION_TYPE:
         dst.type.name        = gcc_helpers::type_name(decl_type);
         dst.type.fundamental = fundamental_type::object;
         break;
      case POINTER_TYPE:
         dst.type.fundamental = fundamental_type::pointer;
         {
            auto pointed_to = TREE_TYPE(decl_type);
            dst.type.name = gcc_helpers::type_name(pointed_to);
         }
         break;
   }
   
   //
   // Extract bitpacking options:
   //
   
   gcc_helpers::for_each_in_list_tree(DECL_ATTRIBUTES(decl), [&dst, &decl, &decl_type](tree attr) {
      std::string_view attr_name;
      {
         auto purpose = TREE_PURPOSE(attr);
         if (purpose == NULL_TREE)
            return;
         attr_name = IDENTIFIER_POINTER(purpose);
      }
      auto argument = TREE_VALUE(attr); // another list tree
      
      if (attr_name == "lu_bitpack_omit") {
         dst.options.do_not_serialize = true;
         return;
      }
      if (attr_name == "lu_bitpack_bitcount") {
         if (!argument)
            return;
         auto value = TREE_VALUE(argument);
         auto opt   = gcc_helpers::extract_integer_constant_from_tree<int>(value);
         if (!opt.has_value()) {
            error(
               "ERROR: bitpacking options for field %s: \"lu_bitpack_bitcount\" attribute "
               "must have an integer constant parameter indicating the number of bits "
               "to pack the field into.",
               dst.name.c_str()
            );
            return;
         }
         int bc = opt.value();
         if (bc <= 0) {
            error(
               "ERROR: bitpacking options for field %s: \"lu_bitpack_bitcount\" attribute "
               "must specify a positive, non-zero bitcount (seen: %d).",
               dst.name.c_str(),
               bc
            );
            return;
         } else if (bc > sizeof(size_t) * 8) {
            warning(
               1,
               "bitpacking options for field %s: \"lu_bitpack_bitcount\" attribute specifies "
               "an unusually large bitcount; is this intentional? (seen: %d)",
               dst.name.c_str(),
               bc
            );
         }
         dst.options.integral.bitcount = bc;
         return;
      }
      if (attr_name == "lu_bitpack_funcs") {
         if (!argument)
            return;
         auto value = TREE_VALUE(argument);
         auto str   = gcc_helpers::extract_string_constant_from_tree(value);
         //
         // TODO
         //
         return;
      }
      if (attr_name == "lu_bitpack_inherit") {
         if (!argument)
            return;
         auto value = TREE_VALUE(argument);
         auto str   = gcc_helpers::extract_string_constant_from_tree(value);
         //
         // TODO
         //
         return;
      }
      if (attr_name == "lu_bitpack_range") {
         if (!argument)
            return;
         intmax_t  min = 0;
         uintmax_t max = 0;
         {
            std::array<tree, 2> extracted;
            gcc_helpers::extract_list_tree_values(argument, extracted);
            if (!extracted[0] || !extracted[1]) {
               return;
            }
            auto opt_min = gcc_helpers::extract_integer_constant_from_tree<intmax_t>(extracted[0]);
            auto opt_max = gcc_helpers::extract_integer_constant_from_tree<uintmax_t>(extracted[1]);
            if (!opt_min.has_value() || !opt_max.has_value()) {
               return;
            }
            min = opt_min.value();
            max = opt_max.value();
         }
         dst.options.integral.min = min;
         dst.options.integral.max = max;
         return;
      }
      if (attr_name == "lu_bitpack_string") {
         if (dst.type.array_indices.empty()) {
            error(
               "ERROR: bitpacking options for field %s: attempted to set string-bitpacking "
               "options on a field that is not a string (array of char).",
               dst.name.c_str()
            );
            return;
         }
         if (!gcc_helpers::type_is_char(decl_type)) {
            error(
               "ERROR: bitpacking options for field %s: attempted to set string-bitpacking "
               "options on a field that is not a string (array of char).",
               dst.name.c_str()
            );
            return;
         }
         
         if (argument != NULL_TREE) {
            auto value = TREE_VALUE(argument);
            auto str   = gcc_helpers::extract_string_constant_from_tree(value);
            
            int  length = 0;
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
               if (length < 0) {
                  error(
                     "ERROR: bitpacking options for field %s: string length cannot be zero or negative (seen: %d)",
                     dst.name.c_str(),
                     length
                  );
                  return;
               }
               dst.options.string.length = length;
               dst.options.string.null_terminated = null_terminated;
            }
         }
         return;
      }
   });
   
   return dst;
}