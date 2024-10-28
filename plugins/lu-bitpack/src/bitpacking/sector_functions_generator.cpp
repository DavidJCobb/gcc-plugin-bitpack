#include "bitpacking/sector_functions_generator.h"
#include "gcc_wrappers/flow/simple_for_loop.h"
#include "gcc_wrappers/builtin_types.h"
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

   void sector_functions_generator::in_progress_func_pair::serialize_array_slice(
      sector_functions_generator& gen,
      const structure_member& info,
      target& object,
      size_t at,
      size_t count
   ) {
      const auto& ty = gw::builtin_types::get_fast();
      {  // read
         gw::flow::simple_for_loop loop(ty.basic_int);
         loop.counter_bounds = {
            .start     = at,
            .last      = at + count - 1,
            .increment = 1,
         };
         gw::statement_list loop_body;
         
         stream_function_set::expression_options expr_opt;
         switch (info.serialization_type) {
            case member_serialization_type::buffer:
               expr_opt.buffer = {
                  .bytecount = info.options.computed.buffer.bytecount,
               };
               break;
            case member_serialization_type::integral:
               expr_opt.integral = {
                  .bitcount = info.options.computed.integral.bitcount,
                  .min      = info.options.computed.integral.min,
               };
               break;
            case member_serialization_type::string:
               expr_opt.string = {
                  .max_length       = info.options.computed.string.length,
                  .needs_terminator = info.options.computed.string.null_terminator,
               };
               break;
         }
         
         static_assert(false, "TODO: different handling needed if array");
         static_assert(false, "TODO: different handling needed if struct");
         loop_body.append(gen.funcs.make_read_expression_for(
            this->read.nth_parameter(0).as_value(),
            object.for_read,
            expr_opt
         ));
         loop_body.append(_read_expr_non_struct(
            gen,
            target.for_read.access_array_element(loop.counter.as_value()),
            info
         ));
         
         loop.bake(std::move(loop_body));
         this->functions.read_root.statements().append(loop.enclosing);
      }
      {  // write
         gw::flow::simple_for_loop loop(ty.basic_int);
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
   void sector_functions_generator::in_progress_func_pair::serialize_entire(sector_functions_generator& gen, target& object) {
      static_assert(false, "TODO");
   }
   
   void sector_functions_generator::in_progress_func_pair::commit() {
      read.set_root_block(read_root);
      save.set_root_block(save_root);
   }
   
   //
   
   void sector_functions_generator::in_progress_sector::next() {
      this->commit();
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
      const auto& ty = gw::builtin_types::get_fast();
      return target{
         .for_read = this->for_read.access_array_element(
            gw::expr::integer_constant(ty.basic_int, n)
         ),
         .for_save = this->for_save.access_array_element(
            gw::expr::integer_constant(ty.basic_int, n)
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
   
   sector_functions_generator::expr_pair sector_functions_generator::_serialize_whole_struct(
      const      structure& info,
      value_pair state_ptr,
      value_pair object
   ) {
      assert(info.type.is_record());
      
      auto& pair = gen;get_or_create_whole_struct_functions(info);
      return expr_pair{
         .read = gw::expr::call(
            pair.read,
            // args:
            state_ptr.for_read,
            object.for_read.address_of()
         ),
         .save = gw::expr::call(
            pair.save,
            // args:
            state_ptr.for_save,
            object.for_save.address_of()
         )
      };
   }
   
   sector_functions_generator::expr_pair sector_functions_generator::_serialize_primitive(
      const      structure_member&,
      value_pair state_ptr,
      value_pair object
   ) {
      expr_pair out;
      
      stream_function_set::expression_options expr_opt;
      switch (info.serialization_type) {
         case member_serialization_type::buffer:
            expr_opt.buffer = {
               .bytecount = info.options.computed.buffer.bytecount,
            };
            break;
         case member_serialization_type::integral:
            expr_opt.integral = {
               .bitcount = info.options.computed.integral.bitcount,
               .min      = info.options.computed.integral.min,
            };
            break;
         case member_serialization_type::string:
            expr_opt.string = {
               .max_length       = info.options.computed.string.length,
               .needs_terminator = info.options.computed.string.null_terminator,
            };
            break;
         default:
            assert(false && "unreachable");
      }
      
      out.read = this->funcs.make_read_expression_for(
         state_ptr.for_read,
         object.for_read,
         expr_opt
      );
      out.save = this->funcs.make_read_expression_for(
         state_ptr.for_save,
         object.for_save,
         expr_opt
      );
      return out;
   }
   
   sector_functions_generator::expr_pair sector_functions_generator::_serialize_array(
      const      structure_member&,
      value_pair state_ptr,
      value_pair object
      size_t     start,
      size_t     count
   ) {
      const auto& ty = gw::builtin_types::get_fast();
      
      gw::flow::simple_for_loop read_loop(ty.basic_int);
      read_loop.counter_bounds = {
         .start     = start,
         .last      = start + count - 1,
         .increment = 1,
      };
      
      gw::flow::simple_for_loop save_loop(ty.basic_int);
      save_loop.counter_bounds = read_loop.counter_bounds;
      
      gw::statement_list read_loop_body;
      gw::statement_list save_loop_body;
      
      std::pair<
         gcc_wrappers::expr::base, // read
         gcc_wrappers::expr::base  // save
      > to_append;
      {
         auto element = target.access_nth(
            read_loop.counter.as_value(),
            save_loop.counter.as_value()
         );
         if (info.type.is_array()) {
            auto extent = info.type.array_extent();
            assert(extent.has_value() && "we don't support VLAs!");
            
            static_assert(false, "TODO: We need to pass the `info` of the nested array instead.");
            to_append = _serialize_array(gen, info, element, 0, *extent);
         } else if (info.type.is_record()) {
            auto* s = gen.discover_struct(info.type);
            assert(s != nullptr);
            to_append = _serialize_whole_struct(gen, *s, element);
         } else {
            to_append = _serialize_primitive(gen, info, element);
         }
      }
      read_loop_body.append(to_append.first);
      save_loop_body.append(to_append.second);
      
      read_loop.bake(std::move(read_loop_body));
      save_loop.bake(std::move(save_loop_body));
      
      return expr_pair{
         .read = read_loop.enclosing,
         .save = save_loop.enclosing
      };
   }
   
   sector_functions_generator::expr_pair sector_functions_generator::_serialize_object(
      value_pair state_ptr,
      value_pair object
   ) {
      const auto& ty = gw::builtin_types::get_fast();
      
      gw::flow::simple_for_loop read_loop(ty.basic_int);
      read_loop.counter_bounds = {
         .start     = start,
         .last      = start + count - 1,
         .increment = 1,
      };
      
      gw::flow::simple_for_loop save_loop(ty.basic_int);
      save_loop.counter_bounds = read_loop.counter_bounds;
      
      gw::statement_list read_loop_body;
      gw::statement_list save_loop_body;
      
      std::pair<
         gcc_wrappers::expr::base, // read
         gcc_wrappers::expr::base  // save
      > to_append;
      {
         auto element = target.access_nth(
            read_loop.counter.as_value(),
            save_loop.counter.as_value()
         );
         if (info.type.is_array()) {
            auto extent = info.type.array_extent();
            assert(extent.has_value() && "we don't support VLAs!");
            
            static_assert(false, "TODO: We need to pass the `info` of the nested array instead.");
            to_append = _serialize_array(gen, info, element, 0, *extent);
         } else if (info.type.is_record()) {
            auto* s = gen.discover_struct(info.type);
            assert(s != nullptr);
            to_append = _serialize_whole_struct(gen, *s, element);
         } else {
            to_append = _serialize_primitive(gen, info, element);
         }
      }
      read_loop_body.append(to_append.first);
      save_loop_body.append(to_append.second);
      
      read_loop.bake(std::move(read_loop_body));
      save_loop.bake(std::move(save_loop_body));
      
      return expr_pair{
         .read = read_loop.enclosing,
         .save = save_loop.enclosing
      };
   }
   
   
   sector_functions_generator::func_pair sector_functions_generator::get_or_create_whole_struct_functions(const structure& info) {
      auto& pair = this->whole_struct_functions[info.type.as_untyped()];
      if (!pair.first.empty()) {
         assert(!pair.second.empty() && "Should've generated both functions together!");
         return pair;
      }
      assert(pair.second.empty() && "Should've generated both functions together!");
      
      const auto& ty = gw::builtin_types::get_fast();
      
      {  // Make the function types and decls first.
         {  // Read
            auto name = std::string("__lu_bitpack_read_") + object_type.name();
            auto type = gw::type::make_function_type(
               types.t_void,
               // args:
               gen.types.stream_state.add_pointer(), // state
               object_type.add_pointer()             // dst
            );
            pair.read = gw::decl::function(name, type);
         }
         {  // Save
            auto name = std::string("__lu_bitpack_write_") + object_type.name();
            auto type = gw::type::make_function_type(
               types.t_void,
               // args:
               gen.types.stream_state.add_pointer(),  // state
               object_type.add_const().add_pointer()  // src
            );
            pair.read = gw::decl::function(name, type);
         }
      }
      auto dst_read = pair.read.as_modifiable();
      auto dst_save = pair.save.as_modifiable();
      dst_read.set_result_decl(gw::decl::result(type.basic_void));
      dst_save.set_result_decl(gw::decl::result(type.basic_void));
      
      gw::expr::local_block root_read;
      gw::expr::local_block root_save;
      
      value_pair state_ptr = {
         .for_read = dst_read.nth_parameter(0).as_value().deference(),
         .for_save = dst_save.nth_parameter(0).as_value().deference(),
      };
      value_pair object = {
         .for_read = dst_read.nth_parameter(1).as_value().deference(),
         .for_save = dst_save.nth_parameter(1).as_value().deference(),
      };
      
      for(auto& m : info.members) {
         auto m_target = object.access_member(m.decl.name());
         auto m_type   = m.type;
         
         std::pair<
            gcc_wrappers::expr::base, // read
            gcc_wrappers::expr::base  // save
         > exprs;
         
         if (m_type.is_array()) {
            auto extent = info.type.array_extent();
            assert(extent.has_value() && "we don't support VLAs!");
            
            static_assert(false, "TODO: We need to pass the `info` of the nested array instead.");
            to_append = _serialize_array(m, state_ptr, m_target, 0, *extent);
         } else if (m_type.is_record()) {
            auto* s = gen.discover_struct(info.type);
            assert(s != nullptr);
            exprs = _serialize_whole_struct(*s, state_ptr, m_target);
         } else {
            assert(!m_type.is_union() && "unions not yet implemented");
            to_append = _serialize_primitive(m, state_ptr, m_target);
         }
         
         assert(!exprs.first.empty());
         assert(!exprs.second.empty());
         root_read.statements().append(exprs.first);
         root_save.statements().append(exprs.second);
      }
      
      dst_read.set_root_block(root_read);
      dst_save.set_root_block(root_save);
      
      return pair;
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
      auto _generate = [this, &sector](target object) -> void {
         auto info_set = this->_info_for_target(object);
         
         // struct:
         if (info_set.struct_info) {
            auto&  info = *info_set.struct_info;
            size_t size = info.compute_packed_bitcount();
            if (size <= bits_remaining) {
               sector.functions.serialize_entire(*this, info, object);
               bits_remaining -= size;
               return;
            }
            for(auto& m : info.members) {
               auto v = object.access_member(m.decl.name());
               _generate(v);
            }
            return;
         }
         
         // non-struct or array of structs:
         if (info_set.member_info) {
            auto&  info = *info_set.member_info;
            size_t size = info.compute_total_packed_bitcount();
            if (size <= bits_remaining) {
               sector.functions.serialize_entire(*this, info, object);
               bits_remaining -= size;
               return;
            }
            if (!info.is_array()) {
               //
               // The most deeply-nested object possible still can't fit. We need to 
               // advance to a sector boundary.
               //
               sector.next();
               //
               // Retry.
               //
               bool result = _generate(object);
               assert(result && "deepest-nested field larger than an entire sector?!");
               return;
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
            return;
         }
         
         assert(false && "unreachable");
         return;
      };

      for(target object : objects_to_serialize) {
         _generate(object);
      }
      sector.functions.commit();
      this->sector_functions.push_back((func_pair)sector.functions); // cast to slice on purpose
   }
}
