#pragma once
#include <memory>
#include <type_traits>
#include <vector>
#include "gcc_wrappers/type/base.h"
#include "codegen/array_access_info.h"
#include "codegen/decl_descriptor.h"

namespace codegen {
   namespace serialization_items {
      class basic_segment;
      class condition;
      class segment;
   }
   class serialization_item;
}

namespace codegen::rechunked::chunks {
   enum class type {
      array_slice,
      condition,
      padding,
      qualified_decl,
      transform,
   };
   
   class base {
      public:
         virtual ~base() {}
         virtual type get_type() const noexcept = 0;
         virtual bool compare(const base&) const noexcept = 0;
         
      public:
         template<typename Subclass> requires std::is_base_of_v<base, Subclass>
         const Subclass* as() const noexcept {
            if (this->get_type() == Subclass::chunk_type)
               return (const Subclass*) this;
            return nullptr;
         }
         
         template<typename Subclass> requires std::is_base_of_v<base, Subclass>
         Subclass* as() noexcept {
            return const_cast<Subclass*>(std::as_const(*this).as<Subclass>());
         }
   };
   
   class transform : public base {
      public:
         static constexpr const type chunk_type = type::transform;
         virtual type get_type() const noexcept override { return chunk_type; }
         
         virtual bool compare(const base&) const noexcept override;
      
      public:
         std::vector<gcc_wrappers::type::base> types;
   };
   
   class qualified_decl : public base {
      public:
         static constexpr const type chunk_type = type::qualified_decl;
         virtual type get_type() const noexcept override { return chunk_type; }
         
         virtual bool compare(const base&) const noexcept override;
      
      public:
         std::vector<const decl_descriptor*> descriptors;
   };
   
   class array_slice : public base {
      public:
         static constexpr const type chunk_type = type::array_slice;
         virtual type get_type() const noexcept override { return chunk_type; }
         
         virtual bool compare(const base&) const noexcept override;
      
      public:
         array_access_info data;
   };
   
   class condition : public base {
      public:
         static constexpr const type chunk_type = type::condition;
         virtual type get_type() const noexcept override { return chunk_type; }
         
         virtual bool compare(const base&) const noexcept override;
      
      public:
         struct _ {
            // Can contain any chunks except for `condition` and `padding` chunks.
            std::vector<std::unique_ptr<base>> chunks;
         } lhs;
         intmax_t rhs = 0;
         
      public:
         void set_lhs_from_segments(const std::vector<serialization_items::basic_segment>&);
         
         bool compare_lhs(const condition& other) const noexcept;
   };
   
   class padding : public base {
      public:
         static constexpr const type chunk_type = type::padding;
         virtual type get_type() const noexcept override { return chunk_type; }
         
         virtual bool compare(const base&) const noexcept override;
         
      public:
         size_t bitcount = 0;
   };
}

namespace codegen::rechunked {
   class item {
      public:
         item() {}
         item(const serialization_item&);
      
      public:
         std::vector<std::unique_ptr<chunks::base>> chunks;
   };
}