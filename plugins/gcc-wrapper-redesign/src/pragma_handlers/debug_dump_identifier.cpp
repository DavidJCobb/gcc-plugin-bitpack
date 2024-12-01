#include "pragma_handlers/debug_dump_identifier.h"
#include <iostream>
#include <string_view>

#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-common.h> // lookup_name

#include <print-tree.h> // debug_tree

#include "gcc_wrappers/decl/base.h"
#include "gcc_wrappers/decl/function.h"
#include "gcc_wrappers/decl/type_def.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/enumeration.h"
#include "gcc_wrappers/type/integral.h"
#include "gcc_wrappers/attribute.h"
#include "gcc_wrappers/identifier.h"
#include "gcc_wrappers/scope.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace pragma_handlers { 
   extern void debug_dump_identifier(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack debug_dump_identifier";
      
      gw::optional_identifier name_opt;
      {
         location_t loc;
         tree       data;
         auto token_type = pragma_lex(&data, &loc);
         if (token_type != CPP_NAME) {
            std::cerr << "error: " << this_pragma_name << ": not a valid identifier\n";
            return;
         }
         name_opt = data;
      }
      auto raw = lookup_name(name_opt.unwrap());
      if (raw == NULL_TREE) {
         //
         // Handle types that have no accompanying DECL node (e.g. RECORD_TYPE 
         // declared directly rather than with the typedef keyword).
         //
         raw = identifier_global_tag(name_opt.unwrap());
         if (raw == NULL_TREE) {
            std::cerr << "error: " << this_pragma_name << ": identifier " << name_opt->name() << " not found\n";
            return;
         }
      }
      
      gw::type::optional_base also_dump;
      if (gw::type::base::raw_node_is(raw)) {
         std::cerr << "Identifier " << name_opt->name() << " is a type.\n";
         
         auto type = gw::type::base::wrap(raw);
         std::cerr << "Pretty-printed: " << type.pretty_print() << '\n';
         
         std::cerr << "Attributes:\n";
         auto list = type.attributes();
         if (list.empty()) {
            std::cerr << " - <none>\n";
         } else {
            for(auto attr : list) {
               std::cerr << " - ";
               std::cerr << attr.name();
               std::cerr << '\n';
            }
         }
         
         std::cerr << "Qualifiers:\n";
         std::cerr << " - Atomic:   " << (type.is_atomic() ? "true" : "false") << '\n';
         std::cerr << " - Const:    " << (type.is_const() ? "true" : "false") << '\n';
         std::cerr << " - Restrict: " << (type.is_restrict() ? "true" : "false") << '\n';
         std::cerr << " - Volatile: " << (type.is_volatile() ? "true" : "false") << '\n';
         std::cerr << " - Allowed to be restrict: " << (type.is_restrict_allowed() ? "true" : "false") << '\n';
         
         std::cerr << "Traits:\n";
         std::cerr << " - Is complete: " << (type.is_complete() ? "true" : "false") << '\n';
         std::cerr << " - Is pointer: " << (type.is_pointer() ? "true" : "false") << '\n';
         std::cerr << " - Is reference: " << (type.is_reference() ? "true" : "false") << '\n';
         std::cerr << "    - lvalue: " << (type.is_lvalue_reference() ? "true" : "false") << '\n';
         std::cerr << "    - rvalue: " << (type.is_rvalue_reference() ? "true" : "false") << '\n';
         std::cerr << " - Alignment: " << type.alignment_in_bits() << " bits (" << type.alignment_in_bytes() << " bytes)\n";
         std::cerr << " - Size: " << type.size_in_bits() << " bits (" << type.size_in_bytes() << " bytes)\n";
         std::cerr << " - Minimum required alignment: " << type.minimum_alignment_in_bits() << " bits\n";
         std::cerr << " - Is packed: " << (type.is_packed() ? "true" : "false") << '\n';
         
         if (type.is_enum()) {
            auto casted  = type.as_enum();
            
            std::cerr << "Enumeration info:\n";
            std::cerr << " - Is opaque: " << (casted.is_opaque() ? "true" : "false") << '\n';
            std::cerr << " - Is scoped: " << (casted.is_scoped() ? "true" : "false") << '\n';
            
            auto members = casted.all_enum_members();
            std::cerr << "Enumeration members:\n";
            if (members.empty()) {
               std::cerr << " - <none>\n";
            } else {
               for(const auto& info : members) {
                  std::cerr << " - " << info.name << " == " << info.value << '\n';
               }
            }
         }
         if (type.is_integral()) {
            auto casted = type.as_integral();
            
            std::cerr << "Integral info:\n";
            std::cerr << " - Signed: " << (casted.is_signed() ? "true" : "false") << '\n';
            std::cerr << " - Min: " << casted.minimum_value() << '\n';
            std::cerr << " - Max: " << casted.maximum_value() << '\n';
            std::cerr << " - Bitcount: " << casted.bitcount() << '\n';
         }
      } else if (gw::decl::base::raw_node_is(raw)) {
         std::cerr << "Identifier " << name_opt->name() << " is a declaration.\n";
         
         auto decl = gw::decl::base::wrap(raw);
         if (gw::decl::type_def::raw_node_is(raw)) {
            std::cerr << "That declaration is a typedef.\n";
            
            auto decl  = gw::decl::type_def::wrap(raw);
            auto prior = decl.is_synonym_of();
            auto after = decl.declared();
            if (prior && after) {
               std::cerr << "Newly-declared name: " << after->pretty_print() << '\n';
               std::cerr << "Is a new synonym of: " << prior->pretty_print() << '\n';
               
               also_dump = after;
            } else {
               std::cerr << "Unable to get complete information. The typedef may still be in the middle of being parsed.\n";
            }
         } else {
            std::cerr << "Name: " << decl.name() << '\n';
         }
         
         std::cerr << "Attributes:\n";
         auto list = decl.attributes();
         if (list.empty()) {
            std::cerr << " - <none>\n";
         } else {
            for(auto attr : list) {
               std::cerr << " - ";
               std::cerr << attr.name();
               std::cerr << '\n';
            }
         }
         
         std::cerr << "Decl properties:\n";
         std::cerr << " - Artificial: " << (decl.is_artificial() ? "true" : "false") << '\n';
         std::cerr << " - Ignored for debugging: " << (decl.is_sym_debugger_ignored() ? "true" : "false") << '\n';
         std::cerr << " - Used: " << (decl.is_used() ? "true" : "false") << '\n';
         
         auto scope = decl.context();
         if (scope) {
            if (scope->is_file_scope()) {
               std::cerr << "Scope: file\n";
            } else {
               auto nf = scope->nearest_function();
               if (nf) {
                  std::cerr << "Scope: function: ";
                  std::cerr << nf->name();
                  std::cerr << '\n';
               } else {
                  auto nt = scope->nearest_type();
                  if (nt) {
                     std::cerr << "Scope: type: ";
                     std::cerr << nt->pretty_print();
                     std::cerr << '\n';
                  }
               }
            }
         }
         
         if (gw::decl::base_value::raw_node_is(raw)) {
            auto decl = gw::decl::base_value::wrap(raw);
            
            auto type = decl.value_type();
            std::cerr << "Value type: " << type.pretty_print() << '\n';
            
            auto init = decl.initial_value();
            if (init) {
               std::cerr << "The declared entity has an initial value.\n";
            }
         }
      }
      
      std::cerr << '\n';
      std::cerr << "Dumping identifier " << name_opt->name() << " as raw data...\n";
      debug_tree(raw);
      
      if (also_dump) {
         std::cerr << "\nDumping typedef-declared type as raw data...\n";
         debug_tree(also_dump.unwrap());
      }
   }
}