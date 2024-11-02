#include "bitpacking/serialization_item.h"
#include "bitpacking/structure.h"

namespace codegen {
   size_t serialization_item::bitcount() const {
      if (!this->member_definition) {
         return this->struct_definition->compute_total_bitcount();
      }

      auto&  extents = this->member_definition->array_extents;
      auto&  indices = this->array_indices;

      size_t element = this->member_definition->compute_single_element_bitcount();
      for (size_t i = indices.size(); i < extents.size(); ++i) {
         element *= extents[i].extent.value;
      }
      return element;
   }
   std::vector<serialization_item*> serialization_item::expand() const {
      std::vector<serialization_item*> out;

      if (this->member_definition) {
         if (this->array_indices.size() < this->member_definition->array_extents.size()) {
            //
            // Expand next array rank, e.g. foo[2] -> foo[2][0], foo[2][1], foo[2][2], foo[2][3]
            //
            size_t next_rank = this->array_indices.size();
            size_t extent = this->member_definition->array_extents[next_rank].extent.value;
            for (size_t i = 0; i < extent; ++i) {
               auto next = new serialization_item(*this);
               next->array_indices.push_back(i);
               out.push_back(next);
            }
            return out;
         }
      }

      const structure* my_type = nullptr;
      if (this->member_definition) {
         static_assert(false, "TODO: if this is a substructure member, set `my_type` to the `structure` object that describes that substructure type");
         if (my_type == nullptr)
            return out;
      } else {
         //
         // We may be missing a member definition if we represent a top-level struct.
         //
         my_type = this->struct_definition;
      }

      for (const auto& m : my_type->members) {
         const auto& member = m;
         /*while (auto* casted = dynamic_cast<const ast::inlined_union_member*>(member)) {
            member = &(casted->get_member_to_serialize());
         }*/

         auto item = new serialization_item;
         item->struct_definition = my_type;
         item->member_definition = member;
         out.push_back(item);
      }

      return out;
   }

   /*static*/ serialization_item serialization_item::for_top_level_struct(const ast::structure& def) {
      serialization_item out;
      out.struct_definition = &def;
      out.member_definition = nullptr;
      return out;
   }

   bool serialization_item::is_array() const {
      if (!this->member_definition)
         return false;
      return this->member_definition->type.is_array();
   }

   bool serialization_item::arg_is_next_array_sibling(const serialization_item& o) const {
      if (!this->member_definition)
         return false;
      if (this->member_definition != o.member_definition)
         return false;
      if (!is_array())
         return false;

      size_t size = this->array_indices.size();
      if (size == 0)
         return false;
      if (size != o.array_indices.size())
         return false;

      for (size_t i = 0; i < size - 1; ++i)
         if (this->array_indices[i] != o.array_indices[i])
            return false;

      return this->array_indices.back() + 1 == o.array_indices.back();
   }
}