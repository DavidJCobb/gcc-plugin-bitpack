#include <iostream>

// Must include gcc-plugin before any other GCC headers.
#include <gcc-plugin.h>
#include <plugin-version.h>

// Required: assert that we're licensed under GPL, or GCC will refuse to run us.
int plugin_is_GPL_compatible;

#include <tree.h>
#include <c-family/c-common.h> // lookup_name
#include <c-family/c-pragma.h>
#include <stringpool.h> // get_identifier
#include "gcc_helpers/dump_function.h"
#include "gcc_helpers/extract_pragma_kv_args.h"

// not allowed to be const
static plugin_info _my_plugin_info = {
   .version = "(unversioned)",
   .help    = "there is no help",
};

#include "handle_struct_type.h"

#include "parse_generate_request.h" // for generate pragmas
#include "generate_read_code.h"

static tree handle_user_attribute(tree* node, tree name, tree args, int flags, bool* no_add_attrs) {
   //
   // The `node` is the node to which the attribute has been applied. If 
   // it's a DECL, you should modify it in place; if it's a TYPE, you 
   // should create a copy (and do what with it?).
   //
   // The `name` is the canonical name of the attribute, i.e. with any 
   // leading or trailing underscores stripped out.
   //
   // The `flags` gives information about the context in which the 
   // attribute was used. The flags are defined in the `attribute_flags` 
   // enum in `tree-core.h`, and are used to indicate what tree type(s) 
   // the attribute is being applied to.
   //
   // If you set `no_add_attrs` to true, then your attribute will not be 
   // added to the DECL_ATTRIBUTES or TYPE_ATTRIBUTES of the target node. 
   // Do this on error or in any other case where your attribute shouldn't 
   // get added.
   //
   // "Depending on FLAGS, any attributes to be applied to another type or 
   // DECL later may be returned; otherwise the return value should be 
   // NULL_TREE.  This pointer may beNULL if no special handling is 
   // required beyond the checks implied by the rest of this structure." 
   // Quoted verbatim because I have no idea what that means.
   //
  return NULL_TREE;
}

namespace _attributes {
   static struct attribute_spec test_attribute = {
      
      // attribute name sans leading or trailing underscores; or NULL to mark 
      // the end of a table of attributes.
      .name = "lu_test_attribute",
      
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount (use -1 for no max)
      .decl_required = false, // if true, applying the attr to a type means it's ignored and warns
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler    = handle_user_attribute,
      
      // Can be an array of `attribute_spec::exclusions` objects, which describe 
      // attributes that this attribute is mutually exclusive with. (How do you 
      // mark the end of the array?)
      .exclude = NULL
   };
   static struct attribute_spec bitpack_bitcount = {
      .name = "lu_bitpack_bitcount",
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount
      .decl_required = true,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = handle_user_attribute,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_funcs = {
      .name = "lu_bitpack_funcs",
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount
      .decl_required = true,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = handle_user_attribute,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_inherit = {
      .name = "lu_bitpack_inherit",
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount
      .decl_required = true,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = handle_user_attribute,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_range = {
      .name = "lu_bitpack_range",
      .min_length = 2, // min argcount
      .max_length = 2, // max argcount
      .decl_required = true,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = handle_user_attribute,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_string = {
      .name = "lu_bitpack_string",
      .min_length = 0, // min argcount
      .max_length = 1, // max argcount
      .decl_required = true,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = handle_user_attribute,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_omit = {
      .name = "lu_bitpack_omit",
      .min_length = 0, // min argcount
      .max_length = 0, // max argcount
      .decl_required = true,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = handle_user_attribute,
      .exclude = NULL
   };
}

static void register_attributes(void* event_data, void* data) {
   //warning (0, G_("Callback to register attributes"));
   register_attribute(&_attributes::test_attribute);
   register_attribute(&_attributes::bitpack_bitcount);
   register_attribute(&_attributes::bitpack_funcs);
   register_attribute(&_attributes::bitpack_inherit);
   register_attribute(&_attributes::bitpack_range);
   register_attribute(&_attributes::bitpack_string);
   register_attribute(&_attributes::bitpack_omit);
}

namespace _pragmas {
   namespace {
      void _dump_pragma_kv(const gcc_helpers::pragma_kv_set& kv) {
         for(const auto& pair : kv) {
            std::cerr << " - " << pair.first << " == ";
            
            const auto& value = pair.second;
            switch (value.index()) {
               case 0:
                  std::cerr << "<empty>";
                  break;
               case 1:
                  std::cerr << "identifier: ";
                  std::cerr << std::get<1>(value);
                  break;
               case 2:
                  std::cerr << "string constant: ";
                  std::cerr << std::get<2>(value);
                  break;
               case 3:
                  std::cerr << "integer constant: ";
                  std::cerr << std::get<3>(value);
                  break;
            }
            std::cerr << '\n';
         }
      }
   }
   
   static void set_options(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack set_options";
      
      const auto kv = gcc_helpers::extract_pragma_kv_args(
         this_pragma_name,
         reader
      );
      std::cerr << "saw #pragma lu_bitpack set_options (not yet implemented); seen KVs are:\n";
      _dump_pragma_kv(kv);
   }
   static void generate_read(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack generate_read";
      
      auto request_opt = parse_generate_request(reader);
      if (request_opt.has_value()) {
         
         static_assert(false, "TODO");
         //
         // I'm not sure if pragmas have enough visibility into the current parse state 
         // to know a) that they're being invoked in a function body and b) be able to 
         // inject code directly into that function body.
         //
         // Worst-case, we may need to have the user forward-declare a function, specify 
         // its identifier as another argument here, and then we can generate the body 
         // of the function.
         //
         
      }
   }
   static void generate_pack(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack generate_pack";
      
      std::cerr << "saw #pragma lu_bitpack generate_pack (not yet implemented)\n";
   }
   
   static void debug_dump_function(cpp_reader* reader) {
      constexpr const char* this_pragma_name = "#pragma lu_bitpack debug_dump_function";
      
      location_t loc;
      tree       data;
      auto token_type = pragma_lex(&data, &loc);
      if (token_type != CPP_NAME) {
         std::cerr << "error: " << this_pragma_name << ": not a valid identifier\n";
         return;
      }
      std::string_view name = IDENTIFIER_POINTER(data);
      
      auto decl = lookup_name(get_identifier(name.data()));
      if (decl == NULL_TREE) {
         std::cerr << "error: " << this_pragma_name << ": identifier " << name << " not found\n";
         return;
      }
      if (TREE_CODE(decl) != FUNCTION_DECL) {
         std::cerr << "error: " << this_pragma_name << ": identifier " << name << " is something other than a function\n";
         return;
      }
      gcc_helpers::dump_function(decl);
   }
}

static void register_pragmas(void* event_data, void* data) {
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "set_options",
      &_pragmas::set_options
   );
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "generate_read",
      &_pragmas::generate_read
   );
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "generate_pack",
      &_pragmas::generate_pack
   );
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "debug_dump_function",
      &_pragmas::debug_dump_function
   );
}

/*static void plugin_finish_type(void *event_data, void *user_data) {
   tree type = (tree)event_data;
   if (!COMPLETE_TYPE_P(type)) {
      //
      // Not a complete type. Skip.
      //
      return;
   }
   if (TREE_CODE(type) == RECORD_TYPE) {
      handle_struct_type(type);
      return;
   }
   if (TREE_CODE(type) == UNION_TYPE) {
      //
      // TODO
      //
      return;
   }
}*/

int plugin_init (
   struct plugin_name_args *plugin_info,
   struct plugin_gcc_version *version
) {
   if (!plugin_default_version_check(version, &gcc_version)) {
      std::cerr << "This plug-in is for version " <<
                   GCCPLUGIN_VERSION_MAJOR << "." <<
                   GCCPLUGIN_VERSION_MINOR << ".\n";
      return 1;
   }
   
   register_callback(
      plugin_info->base_name,
      PLUGIN_INFO,
      /* callback  */ NULL,
      /* user_data */ &_my_plugin_info
   );
   
   std::cerr << "Loaded plug-in: " << plugin_info->base_name << ".\n";
   
   /*register_callback(
      plugin_info->base_name,
      PLUGIN_FINISH_TYPE,
      plugin_finish_type,
      NULL
   );*/
   register_callback(
      plugin_info->base_name,
      PLUGIN_ATTRIBUTES,
      register_attributes,
      NULL
   );
   register_callback(
      plugin_info->base_name,
      PLUGIN_PRAGMAS,
      register_pragmas,
      NULL
   );
   
   return 0;
}