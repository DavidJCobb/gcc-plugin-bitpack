#include "bitpacking/sector_functions_generator.h"
#include "gcc_wrappers/flow/simple_for_loop.h"
#include "gcc_wrappers/statement_list.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace codegen {
   sector_functions_generator::func_pair::func_pair()
   :
      read(gw::decl::function::from_untyped(NULL_TREE)),
      save(gw::decl::function::from_untyped(NULL_TREE))
   {}
   
   //
   
   gw::expr::base sector_functions_generator::_read_expr_non_struct(
      owner_type& owner,
      gcc_wrappers::value dst,
      const structure_member& dst_info
   ) {
      const bool is_buffer = dst_info.serialization_type == member_serialization_type::buffer;
      const bool is_string = dst_info.serialization_type == member_serialization_type::string;
      
      auto bitstream_read = gw::decl::function::from_untyped(NULL_TREE);
      if (is_buffer || is_string) {
         if (is_buffer) {
            bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_buffer.decl);
         } else if (is_string) {
            if (dst_info.options.computed.string.null_terminated) {
               bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_string_wt.decl);
            } else {
               bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_string.decl);
            }
         }
         assert(!bitstream_read.empty());
         
         return gw::expr::call(
            bitstream_read,
            // args:
            this->read.nth_parameter(0).as_value(), // state_ptr
            dst.convert_array_to_pointer(),
            gw::expr::integer_constant(
               gw::type::from_untyped(uint16_type_node),
               dst_info.options.computed.string.length
            )
         );
      }
      
      if (dst_info.serialization_type == member_serialization_type::boolean) {
         if (dst_info.options.computed.integral.bitcount == 1) {
            bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_bool.decl);
            assert(!bitstream_read.empty());
            
            return gw::expr::assign(
               dst,
               // =
               gw::expr::call(
                  bitstream_read,
                  // args:
                  this->read.nth_parameter(0).as_value() // state_ptr
               )
            );
         }
      }
      
      assert(dst_info.serialization_type != member_serialization_type::substructure);
      
      auto dst_type = dst.value_type();
      if (dst_type.canonical() == gw::type::from_untyped(uint8_type_node)) {
         bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_u8.decl);
      } else if (dst_type.canonical() == gw::type::from_untyped(uint16_type_node)) {
         bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_u16.decl);
      } else if (dst_type.canonical() == gw::type::from_untyped(uint32_type_node)) {
         bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_u32.decl);
      } else if (dst_type.canonical() == gw::type::from_untyped(int8_type_node)) {
         bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_s8.decl);
      } else if (dst_type.canonical() == gw::type::from_untyped(int16_type_node)) {
         bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_s16.decl);
      } else if (dst_type.canonical() == gw::type::from_untyped(int32_type_node)) {
         bitstream_read = gw::decl::function::from_untyped(owner.funcs.bitstream_read_s32.decl);
      }
      assert(!bitstream_read.empty());
      
      return gw::expr::assign(
         dst,
         // =
         gw::expr::call(
            bitstream_read,
            // args:
            this->read.nth_parameter(0).as_value(),               // state_ptr
            gw::expr::integer_constant(types.t_uint8_t, bitcount) // bitcount
         )
      );
   }

   void sector_functions_generator::serialize_array_slice(
      sector_functions_generator& gen,
      const structure_member& info,
      target& object,
      size_t at,
      size_t count
   ) {
      {  // read
         gw::flow::simple_for_loop loop(gw::type::from_untyped(integer_type_node));
         loop.counter_bounds = {
            .start     = at,
            .last      = at + count - 1,
            .increment = 1,
         };
         gw::statement_list loop_body;
         
         static_assert(false, "TODO: different handling needed if array of arrays");
         static_assert(false, "TODO: different handling needed if array of structs");
         loop_body.append(_read_expr_non_struct(
            gen,
            target.for_read.access_array_element(loop.counter.as_value()),
            info
         ));
         
         loop.bake(std::move(loop_body));
         this->functions.read_root.statements().append(loop.enclosing);
      }
      {  // write
         gw::flow::simple_for_loop loop(gw::type::from_untyped(integer_type_node));
         loop.counter_bounds = {
            .start     = at,
            .last      = at + count - 1,
            .increment = 1,
         };
         gw::statement_list loop_body;
         {
            static_assert(false, "TODO");
            /*// what we generate will need to look like this:
            // lu_BitstreamWrite_u8(state_ptr, target[n], bitcount);
            gw::expr::call(
               gw::decl::function::from_untyped(needed_functions.bitstream_write_u8.decl),
               // args:
              func_decl.nth_parameter(0).as_value(),          // state_ptr
               target.for_write.access_array_element(loop.counter.as_value()),
               gw::expr::integer_constant(types.t_uint8_t, 6) // bitcount
            )
            //*/
         }
         loop.bake(std::move(loop_body));
         this->functions.write_root.statements().append(loop.enclosing);
      }
   }
   void sector_functions_generator::serialize_entire(sector_functions_generator& gen, target& object) {
      static_assert(false, "TODO");
   }
   
   void sector_functions_generator::in_progress_func_pair::commit() {
      read.set_root_block(read_root);
      save.set_root_block(save_root);
   }
   
   //
   
   void sector_functions_generator::in_progress_sector::next() {
      this->owner.sector_functions.push_back(this->functions);
      ++this->id;
      this->bits_remaining = sector_size;
      static_assert(false, "TODO: make new functions");
      this->functions      = _make_func_pair(id);
   }
   
   //
   
   sector_functions_generator::target sector_functions_generator::target::access_member(std::string_view name) {
      return target{
         .for_read = this->for_read.access_member(name.data()),
         .for_save = this->for_save.access_member(name.data()),
      };
   }
   sector_functions_generator::target sector_functions_generator::target::access_nth(size_t n) {
      return target{
         .for_read = this->for_read.access_array_element(
            gw::expr::integer_constant(
               gw::type::from_untyped(integer_type_node),
               n
            )
         ),
         .for_save = this->for_save.access_array_element(
            gw::expr::integer_constant(
               gw::type::from_untyped(integer_type_node),
               n
            )
         ),
      };
   }
   
   void sector_functions_generator::target::serialize_array_slice(func_pair& functions, size_t at, size_t count);
   void sector_functions_generator::target::serialize_entire(func_pair& functions);
   
   //
   
   sector_functions_generator::~sector_functions_generator() {
      {
         auto& list = this->whole_struct_functions;
         for(auto& pair : list)
            delete pair.second;
         list.clear();
      }
      {
         auto& list = this->types_to_structures;
         for(auto& pair : list)
            delete pair.second;
         list.clear();
      }
   }
   
   sector_functions_generator::target_info sector_functions_generator::_info_for_target(target& tgt) {
      auto bare_node = tgt.as_untyped();
      while (TREE_CODE(bare_node) == ARRAY_REF) {
         bare_node = TREE_OPERAND(bare_node, 0);
      }
      switch (TREE_CODE(bare_node)) {
         case VAR_DECL:
            return target_info{
               .struct_info = discover_struct(tgt.value_type()),
            };
         case COMPONENT_REF:
            {
               gw::value struct_var;
               struct_var.set_from_untyped(TREE_OPERAND(bare_node, 0));
               
               auto struct_type = struct_var.value_type();
               assert(struct_type.is_record());
               
               auto field_decl = gw::decl::field(TREE_OPERAND(bare_node, 1));
               
               auto* info = discover_struct(struct_type);
               for(auto& member : info->members) {
                  if (member.decl == field_decl) {
                     auto vt = field_decl.value_type();
                     if (vt.is_record()) {
                        assert(!vt.is_array());
                        return target_info{
                           .struct_info = info,
                           .member_info = &member
                        };
                     }
                     return target_info{
                        .member_info = &member
                     };
                  }
               }
               assert(false, "unreachable");
            }
            break;
      }
      assert(false && "unreachable");
   }
   
   structure* sector_functions_generator::discover_struct(gw::type st) {
      assert(st.is_record());
      
      auto*& item = this->types_to_structures[st.as_untyped()];
      if (item)
         return item;
      
      item = new structure(st);
      return item;
   }
   void sector_functions_generator::run() {
      in_progress_sector sector(*this);
      
      // returns true if serialized anything, or false if moved to next sector
      auto _generate = [this, &sector](target object) -> bool {
         auto info_set = this->_info_for_target(object);
         
         // struct:
         if (info_set.struct_info) {
            auto&  info = *info_set.struct_info;
            size_t size = info.compute_packed_bitcount();
            if (size <= bits_remaining) {
               sector.functions.serialize_entire(*this, info, object);
               bits_remaining -= size;
               return true;
            }
            for(auto& m : info.members) {
               auto v = object.access_member(m.decl.name());
               if (!_generate(v)) {
                  //
                  // We had to move to the next sector. Try again.
                  //
                  bool result = _generate(member);
                  assert(result && "deepest-nested field larger than an entire sector?!");
               }
            }
            return true;
         }
         
         // non-struct or array of structs:
         if (info_set.member_info) {
            auto&  info = *info_set.member_info;
            size_t size = info.compute_total_packed_bitcount();
            if (size <= bits_remaining) {
               sector.functions.serialize_entire(*this, info, object);
               bits_remaining -= size;
               return true;
            }
            if (!info.is_array()) {
               //
               // The most deeply-nested object possible still can't fit. We need to 
               // advance to a sector boundary.
               //
               sector.next();
               return false;
            }
            
            size_t elem_size = info.compute_single_element_packed_bitcount();
            size_t i         = 0;
            size_t extent    = info.extent();
            while (i < extent) {
               //
               // Generate a loop to serialize as many elements as can fit.
               //
               size_t can_fit = bits_remaining / elem_size;
               if (can_fit > 1) {
                  sector.functions.serialize_array_slice(*this, object, i, can_fit);
                  i += can_fit;
                  if (i >= extent)
                     break;
               }
               //
               // Split the element that won't fit across a sector boundary.
               //
               _generate(object.access_nth(i));
               //
               // Repeat this process until the whole array makes it in.
               //
            }
            return true;
         }
         
         assert(false && "unreachable");
         return true;
      };

      for(target object : objects_to_serialize) {
         _generate(object);
      }
      sector.functions.commit();
      this->sector_functions.push_back((func_pair)sector.functions); // cast to slice on purpose
   }
}
