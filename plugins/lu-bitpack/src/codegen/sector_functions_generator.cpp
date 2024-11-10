#include "codegen/sector_functions_generator.h"
#include <iostream>
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier
#include "bitpacking/global_options.h"
#include "codegen/generate_omitted_default_for_read.h"
#include "gcc_wrappers/decl/result.h"
#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/ternary.h"
#include "gcc_wrappers/flow/simple_for_loop.h"
#include "gcc_wrappers/type/helpers/make_function_type.h"
#include "gcc_wrappers/builtin_types.h"
#include "gcc_wrappers/statement_list.h"
#include "gcc_helpers/stringify_fully_qualified_accessor.h"
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
   
   sector_functions_generator::in_progress_sector::in_progress_sector(sector_functions_generator& o) : owner(o) {
      this->bits_remaining = this->owner.global_options.sectors.size_per * 8;
   }
   void sector_functions_generator::in_progress_sector::assert_sane() const {
      //assert(this->bits_remaining < (size_t)40000000);
   }
   bool sector_functions_generator::in_progress_sector::empty() const {
      return this->bits_remaining == this->owner.global_options.sectors.size_per;
   }
   bool sector_functions_generator::in_progress_sector::has_functions() const {
      if (!this->functions.read.empty()) {
         assert(!this->functions.save.empty());
         return true;
      }
      return false;
   }
   void sector_functions_generator::in_progress_sector::make_functions() {
      const auto& ty = gw::builtin_types::get_fast();
      if (this->function_type.empty()) {
         this->function_type = gw::type::make_function_type(
            ty.basic_void,
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
      this->functions.read.as_modifiable().set_result_decl(gw::decl::result(ty.basic_void));
      this->functions.save.as_modifiable().set_result_decl(gw::decl::result(ty.basic_void));
      
      this->functions.read.nth_parameter(0).make_used();
      this->functions.save.nth_parameter(0).make_used();
      
      this->functions.read_root = {};
      this->functions.save_root = {};
   }
   void sector_functions_generator::in_progress_sector::record_serialized_value_path(const serialization_value_path& path) {
      this->current_report.serialized_object_paths.push_back(path);
   }
   void sector_functions_generator::in_progress_sector::commit_to_owner() {
      this->assert_sane();
      
      // Finalize generated data.
      this->functions.commit();
      
      // Store generated data.
      this->owner.sector_functions.push_back(this->functions);
      this->owner.reports.by_sector.push_back(this->current_report);
   }
   void sector_functions_generator::in_progress_sector::next() {
      this->commit_to_owner();
      
      // Start next sector.
      ++this->id;
      this->bits_remaining = this->owner.global_options.sectors.size_per * 8;
      if (this->id >= this->owner.global_options.sectors.max_count) {
         throw std::runtime_error(lu::strings::printf_string(
            "reached the sector limit (%u) with data still yet to be serialized",
            (int)this->id
         ));
      }
      //
      // ...including starting with fresh data.
      //
      this->current_report = {};
      this->make_functions();
   }
   
   //
   // sector_functions_generator
   //
   
   sector_functions_generator::sector_functions_generator(bitpacking::global_options::computed& go)
      :
      global_options(go) {
   }
   sector_functions_generator::~sector_functions_generator() {
   }
   
   func_pair sector_functions_generator::get_or_create_whole_struct_functions(const struct_descriptor& info) {
      auto& stored = this->_struct_info[info.type];
      assert(stored.descriptor != nullptr);
      
      auto& pair = stored.whole_struct_functions;
      if (!pair.read.empty()) {
         assert(!pair.save.empty() && "Should've generated both functions together!");
         return pair;
      }
      assert(pair.save.empty() && "Should've generated both functions together!");
      
      const auto& ty = gw::builtin_types::get_fast();
      
      {  // Make the function types and decls first.
         {  // Read
            auto name = std::string("__lu_bitpack_read_") + info.type.name();
            auto type = gw::type::make_function_type(
               ty.basic_void,
               // args:
               this->global_options.types.bitstream_state_ptr, // state
               info.type.add_pointer() // dst
            );
            pair.read = gw::decl::function(name, type);
         }
         {  // Save
            auto name = std::string("__lu_bitpack_write_") + info.type.name();
            auto type = gw::type::make_function_type(
               ty.basic_void,
               // args:
               this->global_options.types.bitstream_state_ptr, // state
               info.type.add_const().add_pointer()  // src
            );
            pair.save = gw::decl::function(name, type);
         }
      }
      auto dst_read = pair.read.as_modifiable();
      auto dst_save = pair.save.as_modifiable();
      dst_read.set_result_decl(gw::decl::result(ty.basic_void));
      dst_save.set_result_decl(gw::decl::result(ty.basic_void));
      
      gw::expr::local_block root_read;
      gw::expr::local_block root_save;
      auto read_statements = root_read.statements();
      auto save_statements = root_save.statements();
      
      value_pair state_ptr = {
         .read = dst_read.nth_parameter(0).as_value(),
         .save = dst_save.nth_parameter(0).as_value(),
      };
      
      serialization_value object;
      object.read = dst_read.nth_parameter(1).as_value().dereference();
      object.save = dst_save.nth_parameter(1).as_value().dereference();
      object.descriptor = &info;
      
      for(auto& m_descriptor : info.members) {
         auto m_value = object.access_member(m_descriptor);
         auto expr    = _serialize(state_ptr, m_value);
         
         assert(!expr.read.empty());
         assert(!expr.save.empty());
         read_statements.append(expr.read);
         save_statements.append(expr.save);
      }
      
      // Generate code to assign default values to any omitted members that have them.
      {
         auto expr = generate_omitted_default_for_read::generate_for_members(object.read);
         if (!expr.empty())
            read_statements.append(expr);
      }
      
      dst_read.set_is_defined_elsewhere(false);
      dst_save.set_is_defined_elsewhere(false);
      dst_read.set_root_block(root_read);
      dst_save.set_root_block(root_save);
      
      // expose these identifiers so we can inspect them with our debug-dump pragmas.
      // (in release builds we'll likely want to remove this, so user code can't call 
      // these functions or otherwise access them directly.)
      dst_read.introduce_to_current_scope();
      dst_save.introduce_to_current_scope();
      
      return pair;
   }
     
   sector_functions_generator::struct_info& sector_functions_generator::info_for_struct(gcc_wrappers::type::record t) {
      auto& stored = this->_struct_info[t];
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
      return this->info_for_struct(value.as_member().type().as_record()).descriptor.get();
   }
   
   bool sector_functions_generator::has_descriptor_for(gcc_wrappers::type::record type) const {
      auto it = this->_struct_info.find(type);
      if (it == this->_struct_info.end())
         return false;
      if (it->second.descriptor == nullptr)
         return false;
      return true;
   }
   
   expr_pair sector_functions_generator::_serialize_whole_struct(value_pair state_ptr, serialization_value& value) {
      value.assert_valid();
      
      const auto* descriptor = _descriptor_for_struct(value);
      assert(descriptor != nullptr);
      auto pair = this->get_or_create_whole_struct_functions(*descriptor);
      return expr_pair{
         .read = gw::expr::call(
            pair.read,
            // args:
            state_ptr.read,
            value.read.address_of()
         ),
         .save = gw::expr::call(
            pair.save,
            // args:
            state_ptr.save,
            value.save.address_of()
         )
      };
   }
   
   expr_pair sector_functions_generator::_serialize_array_slice(value_pair state_ptr, serialization_value& value, size_t start, size_t count) {
      const auto& ty = gw::builtin_types::get_fast();
      
      value.assert_valid();
      
      if (value.is_member()) {
         //
         // Try an optimization: if we're serializing the innermost elements of 
         // this array as opaque buffers via read-buffer calls, then just do that 
         // for the entire array rather than looping on each individual element.
         //
         const auto& info = value.as_member();
         if (info.innermost_kind() == bitpacking::member_kind::buffer) {
            auto count = info.count_of_innermost();
            
            const auto& options = info.bitpacking_options().buffer_options();
            return expr_pair{
               .read = gw::expr::call(
                  this->global_options.functions.read.buffer,
                  // args:
                  state_ptr.read,
                  value.read.address_of(),
                  gw::expr::integer_constant(ty.uint16, options.bytecount * count)
               ),
               .save = gw::expr::call(
                  this->global_options.functions.save.buffer,
                  // args:
                  state_ptr.save,
                  value.save.address_of(),
                  gw::expr::integer_constant(ty.uint16, options.bytecount * count)
               )
            };
         }
      }
      
      gw::flow::simple_for_loop read_loop(ty.basic_int);
      read_loop.counter_bounds = {
         .start     = (intmax_t)start,
         .last      = (uintmax_t)(start + count - 1),
         .increment = 1,
      };
      
      gw::flow::simple_for_loop save_loop(ty.basic_int);
      save_loop.counter_bounds = read_loop.counter_bounds;
      
      gw::statement_list read_loop_body;
      gw::statement_list save_loop_body;
      
      expr_pair to_append;
      {
         auto element = value.access_array_slice(value_pair{
            .read = read_loop.counter.as_value(),
            .save = save_loop.counter.as_value()
         });
         to_append = _serialize(state_ptr, element);
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
   
   // Only appropriate for when we know a whole transformed value will fit.
   expr_pair sector_functions_generator::_serialize_transformed(value_pair state_ptr, serialization_value& value) {
      value.assert_valid();
      // TODO: This assertion is bad for top-level structs; find a better way:
      assert(value.is_member());
      const auto& info    = value.as_member();
      const auto& options = info.bitpacking_options().transform_options();
      assert(!options.transformed_type.empty());
      
      /*
      
         Code to generate (read):
         
            {
               TransformedType __transformed_r;
               
               __lu_bitstream_read_TransformedType(state, &__transformed_r);
               
               post_unpack(&value, &__transformed_r);
            }
         
         Code to generate (save):
         
            {
               TransformedType __transformed_s;
               
               pre_pack(&value, &__transformed_s);
               
               __lu_bitstream_save_TransformedType(state, &__transformed_s);
            }
         
      */
      
      expr_pair out;
      {  // Read
         gw::decl::variable transformed("__transformed_r", options.transformed_type);
      
         gw::expr::local_block block;
         auto statements = block.statements();
         statements.append(transformed.make_declare_expr());
         
         // read transformed value:
         // (need to check if it's a struct, array, or primitive)
         // (so really, i think we need to make `serialization_value`s and not just `value`s)
         throw std::runtime_error("not implemented");
         
         statements.append(gw::expr::call(
            options.post_unpack,
            // args:
            value.read.address_of(), // in situ
            transformed.as_value().address_of() // transformed
         ));
         
         out.read = block;
      }
      {  // Save
         gw::decl::variable transformed("__transformed_s", options.transformed_type);
      
         gw::expr::local_block block;
         auto statements = block.statements();
         statements.append(transformed.make_declare_expr());
         
         statements.append(gw::expr::call(
            options.post_unpack,
            // args:
            value.read.address_of(), // in situ
            transformed.as_value().address_of() // transformed
         ));
         
         // save transformed value:
         // (need to check if it's a struct, array, or primitive)
         // (so really, i think we need to make `serialization_value`s and not just `value`s)
         throw std::runtime_error("not implemented");
         
         out.save = block;
      }
      return out;
   }
   
   expr_pair sector_functions_generator::_serialize_primitive(value_pair state_ptr, serialization_value& value) {
      value.assert_valid();
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
         if (options.nonstring) {
            read_func = this->global_options.functions.read.string_ut;
            save_func = this->global_options.functions.save.string_ut;
         } else {
            read_func = this->global_options.functions.read.string_wt;
            save_func = this->global_options.functions.save.string_wt;
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
            read_func = this->global_options.functions.read.u8;
            save_func = this->global_options.functions.save.u8;
         } else if (vt_canonical == ty.uint16) {
            read_func = this->global_options.functions.read.u16;
            save_func = this->global_options.functions.save.u16;
         } else if (vt_canonical == ty.uint32) {
            read_func = this->global_options.functions.read.u32;
            save_func = this->global_options.functions.save.u32;
         } else if (vt_canonical == ty.int8) {
            read_func = this->global_options.functions.read.s8;
            save_func = this->global_options.functions.save.s8;
         } else if (vt_canonical == ty.int16) {
            read_func = this->global_options.functions.read.s16;
            save_func = this->global_options.functions.save.s16;
         } else if (vt_canonical == ty.int32) {
            read_func = this->global_options.functions.read.s32;
            save_func = this->global_options.functions.save.s32;
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
               gw::expr::integer_constant(type.as_integral(), options.min)
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
               to_save = to_save.sub(gw::expr::integer_constant(type.as_integral(), options.min));
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
      } else if (value.is_transformed()) {
         return _serialize_transformed(state_ptr, value);
      } else {
         return _serialize_primitive(state_ptr, value);
      }
   }
   
   void sector_functions_generator::_make_top_level_functions() {
      const auto& ty = gw::builtin_types::get_fast();
      
      in_progress_func_pair top_pair;
      {  // void __lu_bitpack_read_by_sector(const buffer_byte_type* src, int sector_id);
         auto c_name  = this->top_level_function_names.read.c_str();
         auto id_node = get_identifier(c_name);
         auto node    = lookup_name(id_node);
         if (node != NULL_TREE) {
            if (TREE_CODE(node) != FUNCTION_DECL) {
               throw std::runtime_error(lu::strings::printf_string("identifier %<%s%> is already in use by something other than a function", c_name));
            }
            top_pair.read = gw::decl::function::from_untyped(node);
         } else {
            top_pair.read = gw::decl::function(
               c_name,
               gw::type::make_function_type(
                  ty.basic_void,
                  // args:
                  this->global_options.types.buffer_byte_ptr.add_const(),
                  ty.basic_int // int sectorID
               )
            );
            if (top_pair.read.has_body()) {
               throw std::runtime_error(lu::strings::printf_string("cannot generate a definition for function %<%s%>, as it already has a definition", c_name));
            }
         }
      }
      {  // void __lu_bitpack_save_by_sector(buffer_byte_type* dst, int sector_id);
         auto c_name  = this->top_level_function_names.save.c_str();
         auto id_node = get_identifier(c_name);
         auto node    = lookup_name(id_node);
         if (node != NULL_TREE) {
            if (TREE_CODE(node) != FUNCTION_DECL) {
               throw std::runtime_error(lu::strings::printf_string("identifier %<%s%> is already in use by something other than a function", c_name));
            }
            top_pair.save = gw::decl::function::from_untyped(node);
         } else {
            top_pair.save = gw::decl::function(
               c_name,
               gw::type::make_function_type(
                  ty.basic_void,
                  // args:
                  this->global_options.types.buffer_byte_ptr,
                  ty.basic_int // int sectorID
               )
            );
            if (top_pair.save.has_body()) {
               throw std::runtime_error(lu::strings::printf_string("cannot generate a definition for function %<%s%>, as it already has a definition", c_name));
            }
         }
      }
      
      size_t size = this->sector_functions.size();
      {  // Read
         auto statements = top_pair.read_root.statements();
         
         auto state_decl = gw::decl::variable("__lu_bitstream_state", this->global_options.types.bitstream_state);
         state_decl.make_artificial();
         state_decl.make_used();
         //
         statements.append(state_decl.make_declare_expr());
         
         {  // lu_BitstreamInitialize(&state, dst);
            auto src_arg = top_pair.read.nth_parameter(0).as_value();
            {
               auto pointer_type = src_arg.value_type();
               auto value_type   = pointer_type.remove_pointer();
               if (value_type.is_const()) {
                  src_arg = src_arg.conversion_sans_bytecode(value_type.remove_const().add_pointer());
               }
            }
            statements.append(
               gw::expr::call(
                  this->global_options.functions.stream_state_init,
                  // args:
                  state_decl.as_value().address_of(), // &state
                  src_arg // dst
               )
            );
         }
         
         std::vector<gw::expr::call> calls;
         calls.resize(size);
         for(size_t i = 0; i < size; ++i) {
            calls[i] = gw::expr::call(
               this->sector_functions[i].read,
               // args:
               state_decl.as_value().address_of()
            );
         }
         
         auto root_cond = gw::expr::ternary::from_untyped(NULL_TREE);
         auto prev_cond = gw::expr::ternary::from_untyped(NULL_TREE);
         for(size_t i = 0; i < calls.size(); ++i) {
            auto  cond = gw::expr::ternary(
               ty.basic_void,
               
               // condition:
               top_pair.read.nth_parameter(1).as_value().cmp_is_equal(
                  gw::expr::integer_constant(ty.basic_int, i)
               ),
               
               // branches:
               calls[i],
               gw::expr::base::from_untyped(NULL_TREE)
            );
            if (!prev_cond.empty()) {
               prev_cond.set_false_branch(cond);
            } else {
               root_cond = cond;
            }
            prev_cond = cond;
         }
         if (!root_cond.empty()) {
            statements.append(root_cond);
         }
      }
      {  // Save
         auto statements = top_pair.save_root.statements();
         
         auto state_decl = gw::decl::variable("__lu_bitstream_state", this->global_options.types.bitstream_state);
         state_decl.make_artificial();
         state_decl.make_used();
         //
         statements.append(state_decl.make_declare_expr());
         
         {  // lu_BitstreamInitialize(&state, src);
            statements.append(
               gw::expr::call(
                  this->global_options.functions.stream_state_init,
                  // args:
                  state_decl.as_value().address_of(), // &state
                  top_pair.save.nth_parameter(0).as_value() // src
               )
            );
         }
         
         std::vector<gw::expr::call> calls;
         calls.resize(size);
         for(size_t i = 0; i < size; ++i) {
            calls[i] = gw::expr::call(
               this->sector_functions[i].save,
               // args:
               state_decl.as_value().address_of()
            );
         }
         
         auto root_cond = gw::expr::ternary::from_untyped(NULL_TREE);
         auto prev_cond = gw::expr::ternary::from_untyped(NULL_TREE);
         for(size_t i = 0; i < calls.size(); ++i) {
            auto  cond = gw::expr::ternary(
               ty.basic_void,
               
               // condition:
               top_pair.save.nth_parameter(1).as_value().cmp_is_equal(
                  gw::expr::integer_constant(ty.basic_int, i)
               ),
               
               // branches:
               calls[i],
               gw::expr::base::from_untyped(NULL_TREE)
            );
            if (!prev_cond.empty()) {
               prev_cond.set_false_branch(cond);
            } else {
               root_cond = cond;
            }
            prev_cond = cond;
         }
         if (!root_cond.empty()) {
            statements.append(root_cond);
         }
      }
      //
      // Finalize functions:
      //
      {
         auto func_dst = top_pair.read.as_modifiable();
         
         gw::decl::result retn_decl(ty.basic_void);
         func_dst.set_result_decl(retn_decl);
      }
      {
         auto func_dst = top_pair.save.as_modifiable();
         
         gw::decl::result retn_decl(ty.basic_void);
         func_dst.set_result_decl(retn_decl);
      }
      top_pair.read.set_is_defined_elsewhere(false);
      top_pair.save.set_is_defined_elsewhere(false);
      top_pair.commit();
      
      this->top_level_functions = (func_pair)top_pair; // cast for deliberate object slicing
   }
   
   void sector_functions_generator::_serialize_value_to_sector(
      in_progress_sector&      sector,
      serialization_value      object,
      serialization_value_path object_path
   ) {
      sector.assert_sane();
      
      value_pair state_ptr;
      state_ptr.read = sector.functions.read.nth_parameter(0).as_value();
      state_ptr.save = sector.functions.save.nth_parameter(0).as_value();
      
      const size_t bitcount = object.bitcount();
      if (bitcount <= sector.bits_remaining) {
         sector.record_serialized_value_path(object_path);
         sector.functions.append(this->_serialize(state_ptr, object));
         sector.bits_remaining -= bitcount;
         sector.assert_sane();
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
         {
            auto expr = generate_omitted_default_for_read::generate_for_members(object.read);
            if (!expr.empty())
               sector.functions.read_root.statements().append(expr);
         }
         for(const auto& m : info->members) {
            auto v = object.access_member(m);
            this->_serialize_value_to_sector(sector, v, object_path.to_member(m));
         }
         sector.assert_sane();
         return;
      }
      
      //
      // Handle arrays:
      //
      if (object.is_array()) {
         const auto& info = object.as_member();
         //
         // We want to serialize the array in as many contiguous blocks as 
         // possible, using for-loops for those slices that will fit rather 
         // than serializing each element one at a time.
         //
         // NOTE: There's a bit of an edge-case here, if the first array 
         // element can't fit entirely in the current sector. If it's an 
         // indivisible object, then we'll end up generating code that looks 
         // like this after we split to the next sector:
         //
         //    __Lu_bitpack_read_T(state, arr[0]);
         //    for(int i = 1; i < SIZE; ++i) {
         //       __Lu_bitpack_read_T(state, arr[i]);
         //    }
         //
         // This isn't ideal if T is an indivisible type, but we don't really 
         // have a super convenient way to check that from here. (The current 
         // operational definition of an "indivisible type" is any type that 
         // we haven't handled by the time we reach the end of this function, 
         // below this whole branch for handling arrays.)
         //
         size_t elem_size = info.element_size_in_bits();
         size_t i         = 0;
         size_t extent    = info.array_extent();
         while (i < extent) {
            //
            // Generate a loop to serialize as many elements as can fit.
            //
            size_t can_fit = sector.bits_remaining / elem_size;
            if (can_fit > 1) {
               if (i + can_fit > extent) {
                  auto excess = i + can_fit - extent;
                  can_fit -= excess;
               }
               sector.record_serialized_value_path(object_path.to_array_slice(i, can_fit));
               sector.functions.append(
                  this->_serialize_array_slice(state_ptr, object, i, can_fit)
               );
               {
                  size_t consumed = can_fit * elem_size;
                  assert(consumed <= sector.bits_remaining);
                  sector.bits_remaining -= consumed;
                  sector.assert_sane();
               }
               i += can_fit;
               if (i >= extent)
                  break;
            }
            //
            // Split the element that won't fit across a sector boundary.
            //
            this->_serialize_value_to_sector(sector, object.access_nth(i), object_path.to_array_element(i));
            ++i;
            sector.assert_sane();
            //
            // Since we're in a new sector now, update the `state_ptr` to use 
            // the args from the new function. If code in one function refers 
            // to arguments in another function, GCC chokes and dies -- quite 
            // understandably!
            //
            state_ptr.read = sector.functions.read.nth_parameter(0).as_value();
            state_ptr.save = sector.functions.save.nth_parameter(0).as_value();
            //
            // Repeat this process until the whole array makes it in.
            //
         }
         sector.assert_sane();
         return;
      }
      
      //
      // Handle indivisible values. We need to advance to the next sector to fit 
      // them.
      // 
      if (bitcount >= this->global_options.sectors.size_per * 8) {
         auto described = gcc_helpers::stringify_fully_qualified_accessor(object.read);
         auto message   = lu::strings::printf_string(
            "failed to serialize data value %<%s%>: the object is too large to fit in any sector, and is not of a type that can be split across multiple sectors",
            described.c_str()
         );
         throw std::runtime_error(message);
      }
      //
      // Try to open a new sector.
      //
      try {
         sector.next();
         sector.assert_sane();
      } catch (std::runtime_error& ex) {
         //
         // Failed to open a new sector: we've hit the max sector count.
         //
         auto described = gcc_helpers::stringify_fully_qualified_accessor(object.read);
         auto message   = lu::strings::printf_string(
            "failed to serialize data value %<%s%>: ",
            described.c_str()
         );
         message += ex.what();
         //
         throw std::runtime_error(message);
      }
      this->_serialize_value_to_sector(sector, object, object_path);
      sector.assert_sane();
   }
   void sector_functions_generator::run() {
      in_progress_sector sector(*this);
      sector.make_functions();
      
      for(size_t i = 0; i < this->identifiers_to_serialize.size(); ++i) {
         auto& list = identifiers_to_serialize[i];
         if (i > 0 && !sector.empty()) {
            sector.next();
         }
         
         for(auto& name : list) {
            auto node = lookup_name(get_identifier(name.c_str()));
            if (node == NULL_TREE) {
               throw std::runtime_error(lu::strings::printf_string(
                  "failed to find variable %<%s%> to serialize",
                  name.c_str()
               ));
            }
            if (TREE_CODE(node) != VAR_DECL) {
               throw std::runtime_error(lu::strings::printf_string(
                  "identifier %<%s%> does not refer to a variable",
                  name.c_str()
               ));
            }
            
            auto decl = gw::decl::variable::from_untyped(node);
            auto type = decl.value_type();
            if (!type.is_record()) {
               throw std::runtime_error(lu::strings::printf_string(
                  "identifier %<%s%> is not an instance of a struct type (type is %<%s%>)",
                  name.c_str(),
                  type.pretty_print().c_str()
               ));
            }
            
            const auto* descriptor = this->info_for_struct(type.as_record()).descriptor.get();
            
            serialization_value value;
            value.read = decl.as_value();
            value.save = decl.as_value();
            value.descriptor = descriptor;
            
            serialization_value_path value_path;
            value_path.path = name;
            
            this->_serialize_value_to_sector(sector, value, value_path);
         }
      }
      sector.commit_to_owner();
      
      this->_make_top_level_functions();
   }
}