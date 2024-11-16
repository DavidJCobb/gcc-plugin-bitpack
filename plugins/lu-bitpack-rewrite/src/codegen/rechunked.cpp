#include "codegen/rechunked.h"
#include "codegen/serialization_item.h"

namespace codegen::rechunked::chunks {

   //
   // transform
   //
   
   /*virtual*/ bool transform::compare(const base& other) const noexcept /*override*/ {
      const auto* casted = other.as<transform>();
      if (!casted)
         return false;
      
      return this->types == casted->types;
   }
   
   //
   // qualified_decl
   //
   
   /*virtual*/ bool qualified_decl::compare(const base& other) const noexcept /*override*/ {
      const auto* casted = other.as<qualified_decl>();
      if (!casted)
         return false;
      
      return this->descriptors == casted->descriptors;
   }
   
   //
   // array_slice
   //
   
   /*virtual*/ bool array_slice::compare(const base& other) const noexcept /*override*/ {
      const auto* casted = other.as<array_slice>();
      if (!casted)
         return false;
      
      return this->data == casted->data;
   }
   
   //
   // condition
   //
   
   /*virtual*/ bool condition::compare(const base& other) const noexcept /*override*/ {
      const auto* casted = other.as<condition>();
      if (!casted)
         return false;
      
      if (this->rhs != casted->rhs)
         return false;
      if (!this->compare_lhs(*casted))
         return false;
      
      return true;
   }
   
   void condition::set_lhs_from_segments(const std::vector<serialization_items::basic_segment>& src) {
      this->lhs.chunks.clear();
      
      for(const auto& segm : src) {
         bool glued = false;
         if (!this->lhs.chunks.empty()) {
            auto* back = this->lhs.chunks.back()->as<chunks::qualified_decl>();
            if (back) {
               back->descriptors.push_back(segm.desc);
               glued = true;
            }
         }
         if (!glued) {
            auto chunk_ptr = std::make_unique<chunks::qualified_decl>();
            chunk_ptr->descriptors.push_back(segm.desc);
            this->lhs.chunks.push_back(std::move(chunk_ptr));
         }
          
         for(auto& aai : segm.array_accesses) {
            auto chunk_ptr = std::make_unique<chunks::array_slice>();
            chunk_ptr->data = aai;
            this->lhs.chunks.push_back(std::move(chunk_ptr));
         }
         
         if (!segm.desc->types.transformations.empty()) {
            auto chunk_ptr = std::make_unique<chunks::transform>();
            chunk_ptr->types = segm.desc->types.transformations;
            this->lhs.chunks.push_back(std::move(chunk_ptr));
         }
      }
   }
   
   bool condition::compare_lhs(const condition& other) const noexcept {
      size_t size = this->lhs.chunks.size();
      if (size != other.lhs.chunks.size())
         return false;
      for(size_t i = 0; i < size; ++i) {
         auto* a = this->lhs.chunks[i].get();
         auto* b = other.lhs.chunks[i].get();
         assert(a != nullptr);
         assert(b != nullptr);
         if (!a->compare(*b))
            return false;
      }
      return true;
   }
   
   //
   // padding
   //
   
   /*virtual*/ bool padding::compare(const base& other) const noexcept /*override*/ {
      const auto* casted = other.as<padding>();
      if (!casted)
         return false;
      
      return this->bitcount == casted->bitcount;
   }
}

namespace codegen::rechunked {
   
   //
   // item
   //
   
   item::item(const serialization_item& src) {
      for(size_t i = 0; i < src.segments.size(); ++i) {
         const auto& segm   = src.segments[i];
         const bool  is_end = i == src.segments.size() - 1;
         
         if (segm.condition.has_value()) {
            auto& src_cnd = *segm.condition;
            auto  dst_cnd = std::make_unique<chunks::condition>();
            dst_cnd->rhs = src_cnd.rhs;
            dst_cnd->set_lhs_from_segments(src_cnd.lhs);
            this->chunks.push_back(std::move(dst_cnd));
         }
         
         if (segm.is_padding()) {
            if (!is_end) {
               //
               // If padding needs multiple nested conditions, we hack it by 
               // using multiple padding segments wherein only the last one 
               // defines the actual padding size.
               //
               continue;
            }
            auto chunk_ptr = std::make_unique<chunks::padding>();
            chunk_ptr->bitcount = segm.as_padding().bitcount;
            this->chunks.push_back(std::move(chunk_ptr));
            continue;
         }
         assert(segm.is_basic());
         const auto& data = segm.as_basic();
         
         bool glued = false;
         if (!this->chunks.empty()) {
            auto* back = this->chunks.back()->as<chunks::qualified_decl>();
            if (back) {
               back->descriptors.push_back(data.desc);
               glued = true;
            }
         }
         if (!glued) {
            auto chunk_ptr = std::make_unique<chunks::qualified_decl>();
            chunk_ptr->descriptors.push_back(data.desc);
            this->chunks.push_back(std::move(chunk_ptr));
         }
         
         for(auto& aai : data.array_accesses) {
            auto chunk_ptr = std::make_unique<chunks::array_slice>();
            chunk_ptr->data = aai;
            this->chunks.push_back(std::move(chunk_ptr));
         }
         
         const auto& types = data.desc->types.transformations;
         if (!types.empty()) {
            auto chunk_ptr = std::make_unique<chunks::transform>();
            chunk_ptr->types = types;
            this->chunks.push_back(std::move(chunk_ptr));
         }
      }
   }
}