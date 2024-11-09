#include "codegen/generate_omitted_default_for_read.h"
#include "gcc_wrappers/decl/field.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/expr/assign.h"
#include "gcc_wrappers/expr/call.h"
#include "gcc_wrappers/expr/integer_constant.h"
#include "gcc_wrappers/expr/string_constant.h"
#include "gcc_wrappers/flow/simple_for_loop.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/record.h"
#include "gcc_wrappers/builtin_types.h"
#include "debugprint.h"
#include <c-family/c-common.h> // lookup_name
#include <stringpool.h> // get_identifier
namespace gw {
   using namespace gcc_wrappers;
   
   constexpr const char* default_value_attr_name = "lu_bitpack_default_value";
} 

namespace codegen::generate_omitted_default_for_read {
   extern bool is_omitted(gw::decl::field decl) {
      if (decl.attributes().has_attribute("lu_bitpack_omit")) {
         return true;
      }
      auto type = decl.value_type();
      if (type.attributes().has_attribute("lu_bitpack_omit")) {
         return true;
      }
      return false;
   }
   extern bool has_omitted_members(gw::type::record type) {
      bool found = false;
      type.as_record().for_each_referenceable_field([&found](tree node) -> bool {
         auto decl = gw::decl::field::from_untyped(node);
         if (is_omitted(decl)) {
            found = true;
            return false; // break
         }
         return true; // continue
      });
      return found;
   }
   
   extern tree default_value_of(gw::decl::field decl) {
      tree node = NULL_TREE;
      
      auto attr = decl.value_type().attributes().get_attribute("lu_bitpack_default_value");
      if (!attr.empty()) {
         node = attr.arguments().front().as_untyped();
      }
      attr = decl.attributes().get_attribute("lu_bitpack_default_value");
      if (!attr.empty()) {
         node = attr.arguments().front().as_untyped();
      }
      
      return node;
   }
   
   // forward-declare
   static gw::expr::base _for_array(gw::decl::field, gw::value array);
   static gw::expr::base _for_primitive(gw::value, tree src_node);
   
   // Pass `true` for `omitted_uplevel` if the `struct_instance` is an omitted 
   // member of another struct, or anywhere inside of an omitted member of 
   // another struct.
   //
   // If the struct instance is not omitted up-level, then we skip any members 
   // that are struct types unless they are explicitly marked as omitted.
   //
   // In short:
   //
   //    #define LU_BP_OMIT __attribute__((lu_bitpack_omit))
   //
   //    // not omitted up-level
   //    struct TopLevel {
   //       
   //       // not omitted up-level
   //       LU_BP_OMIT struct {
   //       
   //          // omitted up-level
   //          struct {
   //             int c;
   //          } b;
   //          
   //       } a;
   //
   //       // not omitted up-level
   //       struct {
   //       
   //          // not omitted up-level
   //          struct {
   //             int f;
   //          } e;
   //       
   //       } d;
   //
   //    };
   //
   static gw::expr::base _for_struct(gw::value struct_instance, bool omitted_uplevel) {
      auto vt = struct_instance.value_type().as_record();
      if (!has_omitted_members(vt))
         return {};
      
      gw::expr::local_block root;
      
      auto statements = root.statements();
      vt.for_each_referenceable_field(
         [&struct_instance, &statements, omitted_uplevel](tree node) {
            auto decl = gw::decl::field::from_untyped(node);
            if (omitted_uplevel || !is_omitted(decl))
               return;
            
            auto type   = decl.value_type();
            auto member = struct_instance.access_member(decl.name().data());
            
            gw::expr::base expr;
            if (type.is_array()) {
               expr = _for_array(decl, member);
            } else if (type.is_record()) {
               expr = _for_struct(member, true);
            } else if (type.is_union()) {
               assert(false && "not implemented");
            } else {
               auto node = default_value_of(decl);
               if (node != NULL_TREE) {
                  expr = _for_primitive(member, node);
               }
            }
            
            if (!expr.empty())
               statements.append(expr);
         }
      );
      
      return root;
   }
   
   // For an array of [arrays of [...]] structs, when the structs are omitted 
   // uplevel (i.e. they exist entirely inside of an omitted member).
   static gw::expr::base _for_struct_array(gw::value array) {
      const auto& ty = gw::builtin_types::get();
      
      auto array_type = array.value_type().as_array();
      auto value_type = array_type.value_type();
      
      gw::flow::simple_for_loop loop(ty.basic_int);
      loop.counter_bounds = {
         .start     = 0,
         .last      = (uintmax_t) *array_type.extent(),
         .increment = 1,
      };
      
      gw::statement_list loop_body;
      gw::expr::base     expr;
      {
         auto element = array.access_array_element(loop.counter.as_value());
         if (value_type.is_array()) {
            expr = _for_struct_array(element);
         } else {
            expr = _for_struct(element, true);
         }
      }
      loop_body.append(expr);
      
      loop.bake(std::move(loop_body));
      return loop.enclosing;
   }
   
   // For an array of [arrays of [...]] primitives, which will all share the 
   // same constant value node (e.g. the same ADDR_EXPR for a string literal).
   static gw::expr::base _for_primitive_array(gw::value array, tree src_node) {
      const auto& ty = gw::builtin_types::get();
      
      auto array_type = array.value_type().as_array();
      auto value_type = array_type.value_type();
      if (TREE_CODE(src_node) == ADDR_EXPR) { // string literal
         if (!value_type.is_array()) {
            //
            // Use `memcpy` to copy the string literal all at once.
            //
            gw::expr::string_constant str;
            {
               auto cst = TREE_OPERAND(TREE_OPERAND(src_node, 0), 0);
               assert(cst != NULL_TREE && TREE_CODE(cst) == STRING_CST);
               str.set_from_untyped(cst);
            }
            
            gw::value src;
            src.set_from_untyped(src_node);
            
            // memcpy(array, src, sizeof(array));
            return gw::expr::call(
               gw::decl::function::from_untyped(
                  lookup_name(get_identifier("memcpy"))
               ),
               // args:
               array.convert_array_to_pointer(),
               src,
               gw::expr::integer_constant(
                  gw::builtin_types::get().size,
                  str.length() * value_type.size_in_bytes()
               )
            );
         }
      }
      gw::flow::simple_for_loop loop(ty.basic_int);
      loop.counter_bounds = {
         .start     = 0,
         .last      = (uintmax_t) *array_type.extent(),
         .increment = 1,
      };
      
      gw::statement_list loop_body;
      gw::expr::base     expr;
      {
         auto element = array.access_array_element(loop.counter.as_value());
         if (element.value_type().is_array()) {
            expr = _for_primitive_array(element, src_node);
         } else {
            expr = _for_primitive(element, src_node);
         }
      }
      loop_body.append(expr);
      
      loop.bake(std::move(loop_body));
      return loop.enclosing;
   }
   
   // This should be called on an array only after we've checked that it's an 
   // omitted member of a struct.
   static gw::expr::base _for_array(gw::decl::field decl, gw::value array) {
      auto array_type = array.value_type().as_array();
      auto value_type = array_type.value_type();
      while (value_type.is_array()) {
         array_type = value_type.as_array();
         value_type = array_type.value_type();
      }
      //
      // Special-case: array of [arrays of [...]] structs.
      //
      if (value_type.is_record()) {
         return _for_struct_array(array);
      } else if (value_type.is_union()) {
         assert(false && "not implemented");
      }
      //
      // Base case: array of [arrays of [...]] primitives. We pull the default 
      // value out now so that if it requires some transformation (e.g. using 
      // an ADDR_EXPR to access string data, i.e. &__str[0]), we perform that 
      // transformation just once and reuse the expression.
      //
      auto value_node = default_value_of(decl);
      if (value_node == NULL_TREE)
         return {};
      if (TREE_CODE(value_node) == STRING_CST) {
         auto str = gw::expr::string_constant::from_untyped(value_node);
         value_node = str.to_string_literal(value_type).as_untyped();
      }
      return _for_primitive_array(array, value_node);
   }
   
   // Use for something that isn't a struct, array, union, or other container.
   static gw::expr::base _for_primitive(gw::value dst, tree src_node) {
      switch (TREE_CODE(src_node)) {
         case INTEGER_CST:
         case REAL_CST:
            break;
         default:
            assert(false && "unreachable");
      }
      gw::value src;
      src.set_from_untyped(src_node);
      return gw::expr::assign(dst, src);
   }
   
   extern gw::expr::base generate_for_members(gw::value dst) {
      return _for_struct(dst, false);
   }
}