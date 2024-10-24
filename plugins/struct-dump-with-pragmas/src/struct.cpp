#include "struct.h"
#include <string_view>
#include <cp/cp-tree.h> // same_type_p
#include <diagnostic.h> // warning(), error(), etc. (transitively included from diagnostic-core.h)
#include "gcc_helpers/array_extent.h"
#include "gcc_helpers/extract_integer_constant_from_tree.h"
#include "gcc_helpers/extract_string_constant_from_tree.h"
#include "gcc_helpers/type_min_max.h"
#include "gcc_helpers/type_name.h"
#include "gcc_helpers/type_size_in_bytes.h"
#include "handle_kv_string.h"

void Struct::from_gcc_tree(tree type) {
   this->name = gcc_helpers::type_name(type);
   {
      auto opt = gcc_helpers::type_size_in_bytes(type);
      if (opt.has_value())
         this->bytecount = opt.value();
   }
   
   tree fields = TYPE_FIELDS(type);
   for (tree field = fields; field; field = DECL_CHAIN(field)) {
      tree field_type = TREE_TYPE(field);
      
      switch (TREE_CODE(field_type)) {
         default:
            break;
         case METHOD_TYPE:
            continue; // don't track member functions
      }
      
      auto& dst = this->members.emplace_back();
      
      if (TYPE_READONLY(field_type)) {
         dst.type.is_const = true;
      }
      {
         auto d_name = DECL_NAME(field);
         if (d_name) {
            dst.name = IDENTIFIER_POINTER(d_name);
         } else {
            //
            // Anonymous struct?
            //
            dst.name = "<anonymous>";
         }
      }
      if (DECL_BIT_FIELD(field)) {
		   field_type = DECL_BIT_FIELD_TYPE(field);
         {
            auto size = DECL_SIZE(field);
            if (size && TREE_CODE(size) == INTEGER_CST && TREE_CONSTANT(size)) {
               dst.type.bitfield_bitcount = (size_t)TREE_INT_CST_LOW(size);
            }
         }
		}
      
      while (TREE_CODE(field_type) == ARRAY_TYPE) {
         int extent = 0;
         {
            auto opt = gcc_helpers::array_extent(field_type);
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
         field_type = TREE_TYPE(field_type);
      }
      
      if (TREE_CODE(field_type) == REFERENCE_TYPE) {
         dst.type.name        = gcc_helpers::type_name(TREE_TYPE(field_type));
         dst.type.fundamental = fundamental_type::reference;
         continue;
      }
      
      switch (TREE_CODE(field_type)) {
         default:
            dst.type.name        = gcc_helpers::type_name(field_type);
            dst.type.fundamental = fundamental_type::unknown;
            break;
         case ENUMERAL_TYPE:
            dst.type.name        = gcc_helpers::type_name(field_type);
            dst.type.fundamental = fundamental_type::enumeration;
            {
               auto pair = gcc_helpers::type_min_max(field_type);
               dst.type.enum_bounds.min = pair.first.value_or(std::numeric_limits<int64_t>::lowest());
               dst.type.enum_bounds.max = pair.second.value_or(std::numeric_limits<uint64_t>::max());
            }
            break;
         case INTEGER_TYPE:
            dst.type.name        = gcc_helpers::type_name(field_type);
            dst.type.fundamental = fundamental_type::integer;
            {
               auto pair = gcc_helpers::type_min_max(field_type);
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
            dst.type.name        = gcc_helpers::type_name(field_type);
            dst.type.fundamental = fundamental_type::object;
            break;
         case POINTER_TYPE:
            dst.type.fundamental = fundamental_type::pointer;
            {
               auto pointed_to = TREE_TYPE(field_type);
               dst.type.name = gcc_helpers::type_name(pointed_to);
            }
            break;
      }
      //
      // TODO: read attributes to get bitpack options
      //
      
      for(auto attr = DECL_ATTRIBUTES(field); attr != NULL_TREE; attr = TREE_CHAIN(attr)) {
         std::string_view attr_name;
         {
            auto purpose = TREE_PURPOSE(attr);
            if (purpose == NULL_TREE)
               continue;
            attr_name = IDENTIFIER_POINTER(purpose);
         }
         
         auto argument = TREE_VALUE(attr);
         // `argument` is a TREE_LIST
         // To iterate:
         //    for(; argument != NULL_TREE; argument = TREE_CHAIN(argument));
         // TREE_PURPOSE(argument) is a key (null in this case); TREE_VALUE(argument), the value
         
         if (attr_name == "lu_bitpack_omit") {
            dst.options.do_not_serialize = true;
            continue;
         }
         if (attr_name == "lu_bitpack_bitcount") {
            if (!argument)
               continue;
            auto value = TREE_VALUE(argument);
            auto opt   = gcc_helpers::extract_integer_constant_from_tree<int>(value);
            if (!opt.has_value()) {
               error(
                  "ERROR: bitpacking options for field %s: \"lu_bitpack_bitcount\" attribute "
                  "must have an integer constant parameter indicating the number of bits "
                  "to pack the field into.",
                  dst.name.c_str()
               );
               continue;
            }
            int bc = opt.value();
            if (bc <= 0) {
               error(
                  "ERROR: bitpacking options for field %s: \"lu_bitpack_bitcount\" attribute "
                  "must specify a positive, non-zero bitcount (seen: %d).",
                  dst.name.c_str(),
                  bc
               );
               continue;
            }
            if (bc > sizeof(size_t) * 8) {
               warning(
                  1,
                  "bitpacking options for field %s: \"lu_bitpack_bitcount\" attribute specifies "
                  "an unusually large bitcount; is this intentional? (seen: %d)",
                  dst.name.c_str(),
                  bc
               );
            }
            dst.options.integral.bitcount = bc;
            continue;
         }
         if (attr_name == "lu_bitpack_funcs") {
            if (!argument)
               continue;
            auto value = TREE_VALUE(argument);
            auto str   = gcc_helpers::extract_string_constant_from_tree(value);
            //
            // TODO
            //
            continue;
         }
         if (attr_name == "lu_bitpack_inherit") {
            if (!argument)
               continue;
            auto value = TREE_VALUE(argument);
            auto str   = gcc_helpers::extract_string_constant_from_tree(value);
            //
            // TODO
            //
            continue;
         }
         if (attr_name == "lu_bitpack_range") {
            if (!argument)
               continue;
            intmax_t  min = 0;
            uintmax_t max = 0;
            {
               auto value = TREE_VALUE(argument);
               auto opt   = gcc_helpers::extract_integer_constant_from_tree<size_t>(value);
               if (!opt.has_value()) {
                  continue;
               }
               min = opt.value();
            }
            argument = TREE_CHAIN(argument);
            if (argument == NULL_TREE)
               continue;
            {
               auto value = TREE_VALUE(argument);
               auto opt   = gcc_helpers::extract_integer_constant_from_tree<size_t>(value);
               if (!opt.has_value()) {
                  continue;
               }
               max = opt.value();
            }
            dst.options.integral.min = min;
            dst.options.integral.max = max;
            continue;
         }
         if (attr_name == "lu_bitpack_string") {
            if (dst.type.array_indices.empty()) {
               error(
                  "ERROR: bitpacking options for field %s: attempted to set string-bitpacking "
                  "options on a field that is not a string (array of char).",
                  dst.name.c_str()
               );
               continue;
            }
            // TODO: Error if not an array of char
            
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
                     continue;
                  }
                  dst.options.string.length = length;
                  dst.options.string.null_terminated = null_terminated;
               }
            }
            continue;
         }
         
         
      }
   }
}