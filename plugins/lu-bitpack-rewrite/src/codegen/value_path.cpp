#include "codegen/value_path.h"
#include "gcc_wrappers/decl/variable.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/builtin_types.h"
#include "gcc_wrappers/value.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace codegen {
   void value_path::access_array_element(decl_pair pair) {
      this->segments.emplace_back().data.emplace<1>() = pair;
   }
   void value_path::access_array_element(size_t n) {
      this->segments.emplace_back().data.emplace<size_t>() = n;
   }
   void value_path::access_member(const decl_descriptor& desc) {
      this->segments.emplace_back().data.emplace<0>() = decl_pair{
         .read = &desc,
         .save = &desc
      };
   }
   
   void value_path::replace(decl_pair pair) {
      this->segments.clear();
      this->segments.emplace_back().data.emplace<0>() = pair;
   }
   
   value_pair value_path::as_value_pair() const {
      const auto& ty = gw::builtin_types::get();
      
      value_pair out;
      
      bool first = true;
      for(auto& segm : this->segments) {
         if (first) {
            assert(!segm.is_array_access());
            auto& pair = segm.member_descriptor();
            assert(pair.read != nullptr);
            assert(pair.save != nullptr);
            auto decl_r = pair.read->decl;
            auto decl_s = pair.save->decl;
            assert(!decl_r.empty());
            assert(!decl_s.empty());
            assert(decl_r.code() == decl_s.code());
            assert(decl_r.is<gw::decl::param>() || decl_r.is<gw::decl::variable>());
            if (decl_r.is<gw::decl::param>()) {
               out.read = decl_r.as<gw::decl::param>().as_value();
               out.save = decl_s.as<gw::decl::param>().as_value();
            } else {
               out.read = decl_r.as<gw::decl::variable>().as_value();
               out.save = decl_s.as<gw::decl::variable>().as_value();
            }
            first = false;
            continue;
         }
         if (!first) {
            //
            // We may have a value path that looks like `a.b.c.d`, but `a` 
            // may actually be a pointer. This is likely to occur when we 
            // generate whole-struct serialization functions, where the 
            // struct instance is a pointer argument to the function. We 
            // need to produce `a->b.c.d` (or rather `(*a).b.c.d`) rather 
            // than trying to do member access on the pointer itself.
            //
            if (out.read.value_type().is_pointer()) {
               out.read = out.read.dereference();
               out.save = out.save.dereference();
            }
         }
         
         if (segm.is_array_access()) {
            value_pair array_index;
            if (std::holds_alternative<size_t>(segm.data)) {
               auto n = std::get<size_t>(segm.data);
               array_index.read = gw::expr::integer_constant(ty.basic_int, n);
               array_index.save = gw::expr::integer_constant(ty.basic_int, n);
            } else {
               auto& pair = segm.array_loop_counter_descriptor();
               assert(pair.read != nullptr);
               assert(pair.save != nullptr);
               assert(!pair.read->decl.empty());
               assert(!pair.save->decl.empty());
               array_index.read = pair.read->decl.as<gw::decl::variable>().as_value();
               array_index.save = pair.save->decl.as<gw::decl::variable>().as_value();
            }
            out.read = out.read.access_array_element(array_index.read);
            out.save = out.save.access_array_element(array_index.save);
         } else {
            auto& pair = segm.member_descriptor();
            assert(pair.read != nullptr);
            assert(pair.save != nullptr);
            assert(!pair.read->decl.empty());
            assert(!pair.save->decl.empty());
            auto name = pair.read->decl.name();
            out.read = out.read.access_member(name.data());
            out.save = out.save.access_member(name.data());
         }
      }
      
      return out;
   }
   const bitpacking::data_options::computed& value_path::bitpacking_options() const {
      const decl_descriptor* desc = nullptr;
      for(auto& segm : this->segments) {
         if (segm.is_array_access())
            continue;
         desc = segm.member_descriptor().read;
      }
      return desc->options;
   }
}