#include "struct.h"
#include <cp/cp-tree.h> // same_type_p
#include "gcc_helpers/array_extent.h"
#include "gcc_helpers/type_min_max.h"
#include "gcc_helpers/type_name.h"
#include "gcc_helpers/type_size_in_bytes.h"

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
   }
}