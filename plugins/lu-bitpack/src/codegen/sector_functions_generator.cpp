#include "codegen/sector_functions_generator.h"
#include "gcc_wrappers/flow/simple_for_loop.h"
#include "gcc_wrappers/builtin_types.h"
#include "gcc_wrappers/statement_list.h"
#include "lu/strings/printf_string.h"
namespace {
   namespace gw {
      using namespace gcc_wrappers;
   }
}

namespace codegen {
   
   //
   // sector_functions_generator::in_progress_sector
   //
   
   bool sector_functions_generator::in_progress_sector::has_functions() const {
      if (!this->functions.read.empty()) {
         assert(!this->functions.save.empty());
         return true;
      }
      return false;
   }
   void sector_functions_generator::in_progress_sector::make_functions() {
      if (this->function_type.empty()) {
         this->function_type = gw::type::make_function_type(
            types.t_void,
            // args:
            this->owner.global_options.types.bitstream_state_ptr
         );
      }
      
      {  // Read
         auto name = lu::strings::printf_string("__lu_bitpack_read_sector_%u", this->id);
         this->functions.read = gw::decl::function(name, this->function_type);
      }
      {  // Save
         auto name = lu::strings::printf_string("__lu_bitpack_save_sector_%u", this->id);
         this->functions.save = gw::decl::function(name, this->function_type);
      }
   }
   void sector_functions_generator::in_progress_sector::next() {
      this->commit();
      this->owner.sector_functions.push_back(this->functions);
      ++this->id;
      this->bits_remaining = sector_size;
      this->make_functions();
   }
   
   //
   // sector_functions_generator
   //
   
   sector_functions_generator::~sector_functions_generator() {
   }
   
   func_pair sector_functions_generator::get_or_create_whole_struct_functions(const struct_descriptor& info) {
      auto stored = this->_struct_info[info.type];
      assert(stored.descriptor != nullptr);
      
      auto& pair = stored.whole_struct_functions[info.type.as_untyped()];
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
            pair.save = gw::decl::function(name, type);
         }
      }
      auto dst_read = pair.read.as_modifiable();
      auto dst_save = pair.save.as_modifiable();
      dst_read.set_result_decl(gw::decl::result(type.basic_void));
      dst_save.set_result_decl(gw::decl::result(type.basic_void));
      
      gw::expr::local_block root_read;
      gw::expr::local_block root_save;
      
      value_pair state_ptr = {
         .read = dst_read.nth_parameter(0).as_value().deference(),
         .save = dst_save.nth_parameter(0).as_value().deference(),
      };
      
      serialization_value object;
      object.read = dst_read.nth_parameter(1).as_value().deference();
      object.save = dst_save.nth_parameter(1).as_value().deference();
      object.descriptor = &info;
      
      for(auto& m_descriptor : info.members) {
         auto m_value = object.access_member(m_descriptor);
         auto expr    = _serialize(state_ptr, m_value);
         
         assert(!expr.read.empty());
         assert(!expr.save.empty());
         root_read.statements().append(expr.read);
         root_save.statements().append(expr.save);
      }
      
      dst_read.set_root_block(root_read);
      dst_save.set_root_block(root_save);
      
      return pair;
   }
     
   sector_functions_generator::struct_info& sector_functions_generator::info_for_struct(gcc_wrappers::type t) {
      auto stored = this->_struct_info[info.type];
      if (stored.descriptor != nullptr)
         return stored;
      
      stored.descriptor = std::make_unique<struct_descriptor>(this->global_options, t);
      
      return stored;
   }
   
   const struct_descriptor* sector_functions_generator::_descriptor_for_struct(serialization_value& value) {
      assert(value.is_struct());
      if (value.is_top_level_struct()) {
         return &value.as_top_level_struct();
      }
      return this->info_for_struct(object.as_member().type()).descriptor.get();
   }
   
   expr_pair sector_functions_generator::_serialize_whole_struct(value_pair state_ptr, serialization_value& value) {
      const auto* descriptor = _descriptor_for_struct(value);
      assert(descriptor != nullptr);
      auto& pair = this->get_or_create_whole_struct_functions(*descriptor);
      return expr_pair{
         .read = gw::expr::call(
            pair.read,
            // args:
            state_ptr.read,
            object.read.address_of()
         ),
         .save = gw::expr::call(
            pair.save,
            // args:
            state_ptr.save,
            object.save.address_of()
         )
      };
   }
   
   expr_pair sector_functions_generator::_serialize_array_slice(value_pair state_ptr, serialization_value& value, size_t start, size_t count) {
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
      
      expr_pair to_append;
      {
         auto element = value.access_nth(
            read_loop.counter.as_value(),
            save_loop.counter.as_value()
         );
         to_append = _serialize(element);
      }
      read_loop_body.append(to_append.read);
      save_loop_body.append(to_append.save);
      
      read_loop.bake(std::move(read_loop_body));
      save_loop.bake(std::move(save_loop_body));
      
      return expr_pair{
         .read = read_loop.enclosing,
         .save = save_loop.enclosing
      };
   }
   
   expr_pair sector_functions_generator::_serialize_primitive(value_pair state_ptr, serialization_value& value) {
      assert(value.is_member());
      const auto& info = value.as_member();
      assert(!info.is_array());
      
      const auto& ty = gw::builtin_types::get_fast();
      
      auto type = info.type();
      auto kind = info.kind();
      
      assert(kind != bitpacking::member_kind::none);
      assert(kind != bitpacking::member_kind::array);
      assert(kind != bitpacking::member_kind::structure);
      
      //
      // Buffers and strings.
      //
      
      if (kind == bitpacking::member_kind::buffer) {
         const auto& options = info.bitpacking_options().buffer_options();
         return expr_pair{
            .read = gw::expr::call(
               this->global_options.functions.read.buffer,
               // args:
               state_ptr.read,
               value.read.address_of(),
               gw::expr::integer_constant(ty.uint16, options.bytecount)
            ),
            .save = gw::expr::call(
               this->global_options.functions.save.buffer,
               // args:
               state_ptr.save,
               value.save.address_of(),
               gw::expr::integer_constant(ty.uint16, options.bytecount)
            )
         };
      }
      if (kind == bitpacking::member_kind::string) {
         const auto& options = info.bitpacking_options().string_options();
         
         gw::decl::function read_func;
         gw::decl::function save_func;
         if (options.with_terminator) {
            read_func = this->global_options.functions.read.string_wt;
            save_func = this->global_options.functions.save.string_wt;
         } else {
            read_func = this->global_options.functions.read.string_ut;
            save_func = this->global_options.functions.save.string_ut;
         }
         
         return expr_pair{
            .read = gw::expr::call(
               read_func,
               // args:
               state_ptr.read,
               value.read.convert_array_to_pointer(),
               gw::expr::integer_constant(ty.uint16, options.length)
            ),
            .save = gw::expr::call(
               save_func,
               // args:
               state_ptr.save,
               value.save.convert_array_to_pointer(),
               gw::expr::integer_constant(ty.uint16, options.length)
            )
         };
      }
      
      //
      // Integrals below.
      //
      
      switch (kind) {
         case bitpacking::member_kind::boolean:
         case bitpacking::member_kind::integer:
         case bitpacking::member_kind::pointer:
            break;
         default:
            assert(false && "NOT IMPLEMENTED");
      }
      
      gw::decl::function read_func;
      gw::decl::function save_func;
      const auto& options = info.bitpacking_options().integral_options();
      
      expr_pair out;
      
      if (type.is_pointer()) {
         switch (type.size_in_bits()) {
            case 8:
               read_func = this->global_options.functions.read.u8;
               save_func = this->global_options.functions.save.u8;
               break;
            case 16:
               read_func = this->global_options.functions.read.u16;
               save_func = this->global_options.functions.save.u16;
               break;
            case 32:
               read_func = this->global_options.functions.read.u32;
               save_func = this->global_options.functions.save.u32;
               break;
            default:
               assert(false && "unsupported pointer size");
         }
      } else {
         if (options.bitcount == 1 && options.min == 0) {
            //
            // Special-case: 1-bit booleans.
            //
            return expr_pair{
               .read = gw::expr::assign(
                  value.read,
                  gw::expr::call(
                     this->global_options.functions.read.boolean,
                     // args:
                     state_ptr.read
                  )
               ),
               .save = gw::expr::call(
                  this->global_options.functions.save.boolean,
                  // args:
                  state_ptr.save,
                  value.save
               )
            };
         }
         
         auto vt_canonical = type.canonical();
         if (vt_canonical == ty.uint8) {
            read_func = this->read.u8;
            save_func = this->save.u8;
         } else if (vt_canonical == ty.uint16) {
            read_func = this->read.u16;
            save_func = this->save.u16;
         } else if (vt_canonical == ty.uint32) {
            read_func = this->read.u32;
            save_func = this->save.u32;
         } else if (vt_canonical == ty.int8) {
            read_func = this->read.s8;
            save_func = this->save.s8;
         } else if (vt_canonical == ty.int16) {
            read_func = this->read.s16;
            save_func = this->save.s16;
         } else if (vt_canonical == ty.int32) {
            read_func = this->read.s32;
            save_func = this->save.s32;
         }
         assert(!read_func.empty());
         assert(!save_func.empty());
      }
      
      {  // Read
         gw::value to_assign = gw::expr::call(
            read_func,
            // args:
            state_ptr.read,
            gw::expr::integer_constant(ty.uint8, options.bitcount)
         );
         if (type.is_pointer()) {
            assert(options.min == 0);
            to_assign = to_assign.conversion_sans_bytecode(type);
         } else if (options.min != 0) {
            //
            // If a field's range of valid values is [a, b], then serialize it as (v - a), 
            // and then unpack it as (v + a). This means we don't need a sign bit when we 
            // serialize negative numbers, and it also means that ranges that start above 
            // zero don't waste the low bits.
            //
            to_assign = to_assign.add(
               gw::expr::integer_constant(vt, options.min)
            );
         }
         out.read = gw::expr::assign(value.read, to_assign);
      }
      {  // Save
         auto to_save = value.save;
         if (type.is_pointer()) {
            assert(options.min == 0);
            to_save = to_save.conversion_sans_bytecode(ty.smallest_integral_for(type.size_in_bits(), false));
         } else {
            if (options.min != 0)
               to_save = to_save.sub(gw::expr::integer_constant(type, options.min));
         }
         out.save = gw::expr::call(
            save_func,
            // args:
            state_ptr.save,
            to_save,
            gw::expr::integer_constant(ty.uint8, options.bitcount)
         );
      }
      
      return out;
   }
   
   expr_pair sector_functions_generator::_serialize(value_pair state_ptr, serialization_value& value) {
      if (value.is_struct()) {
         return _serialize_whole_struct(state_ptr, value);
      } else if (value.is_array()) {
         return _serialize_array_slice(state_ptr, value, 0, value.array_extent());
      } else {
         return _serialize_primitive(state_ptr, value);
      }
   }
   
   void sector_functions_generator::run() {
      in_progress_sector sector(*this);
      sector.make_functions();
      
      // returns true if serialized anything, or false if moved to next sector
      auto _generate = [this, &sector](serialization_value object) -> void {
         auto info_set = this->_info_for_target(object);
         
         size_t bitcount = object.bitcount();
         if (bitcount <= sector.bits_remaining) {
            sector.functions.append(this->_serialize(object));
            bits_remaining -= size;
            return;
         }
         
         //
         // The object doesn't fit. We need to serialize it in fragments, 
         // splitting it across a sector boundary.
         //
         
         //
         // Handle top-level and nested struct members:
         //
         if (object.is_struct()) {
            const auto* info = _descriptor_for_struct(object);
            assert(info != nullptr);
            for(const auto& m : info->members) {
               auto v = object.access_member(m);
               _generate(v);
            }
            return;
         }
         
         //
         // Handle arrays:
         //
         if (object.is_array()) {
            const auto& info = object.as_member();
            
            size_t elem_size = info.element_size_in_bits();
            size_t i         = 0;
            size_t extent    = info.array_extent();
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
         
         //
         // Handle indiviisible values. We need to advance to the next sector to fit 
         // them.
         // 
         sector.next();
         _generate(object);
         return;
      };

      static_assert(false, "TODO: Where to get `objects_to_serialize` from?");
      for(serialization_value object : objects_to_serialize) {
         _generate(object);
      }
      sector.functions.commit();
      this->sector_functions.push_back((func_pair)sector.functions); // cast to slice on purpose
   }
}