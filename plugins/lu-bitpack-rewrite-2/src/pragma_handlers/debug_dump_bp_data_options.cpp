#include "pragma_handlers/debug_dump_identifier.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <string_view>

#include <gcc-plugin.h>
#include <tree.h>
#include <c-family/c-common.h> // lookup_name

#include "bitpacking/data_options.h"

#include "gcc_wrappers/decl/base_value.h"
#include "gcc_wrappers/type/base.h"
#include "gcc_wrappers/type/container.h"
#include "gcc_wrappers/identifier.h"
#include "gcc_wrappers/scope.h"
namespace gw {
   using namespace gcc_wrappers;
}

constexpr const char* this_pragma_name = "#pragma lu_bitpack debug_dump_bp_data_options";

static std::string _fully_qualified_name(const std::vector<gw::identifier>& names, size_t up_to = std::numeric_limits<size_t>::max()) {
   size_t end = names.size();
   if (end > up_to)
      end = up_to;
   
   std::string fqn;
   for(size_t i = 0; i < end; ++i) {
      if (i > 0)
         fqn += "::";
      fqn += names[i].name();
   }
   return fqn;
}

static gw::optional_node _lookup_path(const std::vector<gw::identifier>& names) {
   assert(!names.empty());
   
   gw::optional_node subject;
   
   //
   // Start by looking up the top-level identifier.
   //
   auto id = names.front();
   subject = lookup_name(id.unwrap());
   if (!subject) {
      //
      // Handle types that have no accompanying DECL node (e.g. RECORD_TYPE 
      // declared directly rather than with the typedef keyword).
      //
      subject = identifier_global_tag(id.unwrap());
      if (!subject) {
         std::cerr << "error: " << this_pragma_name << ": identifier " << id.name() << " not found\n";
         return {};
      }
   }
   
   //
   // Resolve nested names.
   //
   for(size_t i = 1; i < names.size(); ++i) {
      gw::type::optional_container cont;
      if (subject->is<gw::type::container>()) {
         cont = subject->as<gw::type::container>();
      } else if (subject->is<gw::decl::base_value>()) {
         auto vt = subject->as<gw::decl::base_value>().value_type();
         if (vt.is_container()) {
            cont = vt.as_container();
         }
      }
      if (!cont) {
         auto fqn = _fully_qualified_name(names, i);
         std::cerr << "error: " << this_pragma_name << ": identifier " << fqn.c_str() << " does not name a struct or union type (or a field or variable of such type), so we cannot access its members\n";
         return {};
      }
      
      gw::optional_node memb;
      for(auto item : cont->member_chain()) {
         if (item.is<gw::decl::base_value>()) {
            auto cast = item.as<gw::decl::base_value>();
            auto name = cast.name();
            if (name == names[i].name()) {
               memb = cast;
               break;
            }
         } else if (item.is<gw::type::container>()) {
            auto cast = item.as<gw::type::container>();
            auto name = cast.name();
            if (name == names[i].name()) {
               memb = cast;
               break;
            }
         }
      }
      if (!memb) {
         auto fqn = _fully_qualified_name(names, i);
         std::cerr << "error: " << this_pragma_name << ": identifier " << fqn.c_str() << " names a struct or union type which does not contain a nested type or member variable (static or non-static) named " << names[i].name() << "\n";
         return {};
      }
      
      subject = memb;
   }
   
   return subject;
}

namespace pragma_handlers { 
   extern void debug_dump_bp_data_options(cpp_reader* reader) {
      std::vector<gw::identifier> names;
      {
         location_t loc;
         tree       data;
         do {
            auto token_type = pragma_lex(&data, &loc);
            if (token_type == CPP_EOF)
               break;
            if (!names.empty()) {
               if (token_type != CPP_SCOPE) {
                  std::cerr << "error: " << this_pragma_name << ": expected token %<::%> or end of line";
                  return;
               }
               token_type = pragma_lex(&data, &loc);
            }
            if (token_type != CPP_NAME) {
               std::cerr << "error: " << this_pragma_name << ": expected an identifier";
               if (!names.empty()) {
                  std::cerr << " after " << names.back().name();
               }
               if (data != NULL_TREE) {
                  std::cerr << "; saw ";
                  std::cerr << gw::optional_node(data)->code_name();
                  std::cerr << " instead";
               }
               std::cerr << '\n';
               return;
            }
            names.push_back(gw::identifier::wrap(data));
         } while (true);
         if (names.empty()) {
            std::cerr << "error: " << this_pragma_name << ": no identifier given\n";
            return;
         }
      }
      
      auto subject = _lookup_path(names);
      if (!subject)
         return;
      
      auto fqn = _fully_qualified_name(names);
      
      bitpacking::data_options options;
      options.config.report_errors = false;
      if (subject->is<gw::type::base>()) {
         options.load(subject->as<gw::type::base>());
      } else if (subject->is<gw::decl::field>()) {
         options.load(subject->as<gw::decl::field>());
      } else if (subject->is<gw::decl::variable>()) {
         options.load(subject->as<gw::decl::variable>());
      } else {
         std::cerr << "error: " << this_pragma_name << ": identifier " << fqn << " does not appear to be something that can have bitpacking data options\n";
         return;
      }
      
      std::cerr << "Printing data options for " << fqn << "...\n";
      if (!options.valid())
         std::cerr << "(Note: options were not valid.)\n";
      
      if (options.default_value) {
         std::cerr << " - Default value of type " << options.default_value->code_name() << '\n';
      }
      if (options.is_omitted) {
         std::cerr << " - Omitted from bitpacking\n";
      }
      if (!options.stat_categories.empty()) {
         std::cerr << " - Stat-tracking categories:\n";
         for(const auto& name : options.stat_categories)
            std::cerr << "    - " << name << '\n';
      }
      if (options.union_member_id.has_value()) {
         std::cerr << " - Union member ID: " << *options.union_member_id << '\n';
      }
      
      //
      // Print typed options.
      //
      
      if (options.is<bitpacking::typed_data_options::computed::boolean>()) {
         std::cerr << " - Bitpacking type: boolean\n";
      } else if (options.is<bitpacking::typed_data_options::computed::buffer>()) {
         const auto& src = options.as<bitpacking::typed_data_options::computed::buffer>();
         std::cerr << " - Bitpacking type: opaque buffer\n";
         std::cerr << "    - Bytecount: " << src.bytecount << '\n';
      } else if (options.is<bitpacking::typed_data_options::computed::integral>()) {
         const auto& src = options.as<bitpacking::typed_data_options::computed::integral>();
         std::cerr << " - Bitpacking type: integral\n";
         std::cerr << "    - Bitcount: " << src.bitcount << '\n';
         std::cerr << "    - Min:      " << src.min << '\n';
         if (src.max == bitpacking::typed_data_options::computed::integral::no_maximum) {
            std::cerr << "    - Max:      <none>\n";
         } else {
            std::cerr << "    - Max:      " << src.max << '\n';
         }
      } else if (options.is<bitpacking::typed_data_options::computed::pointer>()) {
         std::cerr << " - Bitpacking type: pointer\n";
      } else if (options.is<bitpacking::typed_data_options::computed::string>()) {
         const auto& src = options.as<bitpacking::typed_data_options::computed::string>();
         std::cerr << " - Bitpacking type: string\n";
         std::cerr << "    - Length:    " << src.length << '\n';
         std::cerr << "    - Nonstring: " << (src.nonstring ? "yes" : "no") << '\n';
      } else if (options.is<bitpacking::typed_data_options::computed::structure>()) {
         std::cerr << " - Bitpacking type: struct\n";
      } else if (options.is<bitpacking::typed_data_options::computed::tagged_union>()) {
         const auto& src = options.as<bitpacking::typed_data_options::computed::tagged_union>();
         std::cerr << " - Bitpacking type: union\n";
         std::cerr << "    - Tag identifier:    " << src.tag_identifier << '\n';
         std::cerr << "    - Internally tagged: " << (src.is_internal ? "yes" : "no") << '\n';
      } else if (options.is<bitpacking::typed_data_options::computed::transformed>()) {
         const auto& src = options.as<bitpacking::typed_data_options::computed::transformed>();
         std::cerr << " - Bitpacking type: transformed\n";
         {
            std::cerr << "    - Transformed type: ";
            if (src.transformed_type) {
               auto name = src.transformed_type->name();
               if (name.empty())
                  std::cerr << "<unnamed>";
               else
                  std::cerr << name;
            } else {
               std::cerr << "<none>";
            }
            std::cerr << '\n';
         }
         {
            std::cerr << "    - Pack function: ";
            if (src.pre_pack) {
               auto name = src.pre_pack->name();
               if (name.empty())
                  std::cerr << "<unnamed>";
               else
                  std::cerr << name;
            } else {
               std::cerr << "<none>";
            }
            std::cerr << '\n';
         }
         {
            std::cerr << "    - Unpack function: ";
            if (src.post_unpack) {
               auto name = src.post_unpack->name();
               if (name.empty())
                  std::cerr << "<unnamed>";
               else
                  std::cerr << name;
            } else {
               std::cerr << "<none>";
            }
            std::cerr << '\n';
         }
      } else {
         std::cerr << " - Bitpacking type: unknown\n";
      }
      
      //
      // TODO: Print where options are inherited from (e.g. type(def)s)?
      //
      
      
   }
}