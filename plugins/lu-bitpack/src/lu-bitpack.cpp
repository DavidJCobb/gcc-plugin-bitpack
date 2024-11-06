#include <iostream>
#include <stdexcept>

// Must include gcc-plugin before any other GCC headers.
#include <gcc-plugin.h>
#include <plugin-version.h>

// Required: assert that we're licensed under GPL, or GCC will refuse to run us.
int plugin_is_GPL_compatible;

#include <tree.h>
#include <c-family/c-pragma.h>
#include "gcc_wrappers/builtin_types.h"

// not allowed to be const
static plugin_info _my_plugin_info = {
   .version = "(unversioned)",
   .help    = "there is no help",
};

#include "attribute_handlers/bitpack_bitcount.h"
#include "attribute_handlers/bitpack_range.h"
#include "attribute_handlers/generic_type_or_decl.h"
#include "attribute_handlers/no_op.h"

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
      .handler    = &attribute_handlers::no_op,
      
      // Can be an array of `attribute_spec::exclusions` objects, which describe 
      // attributes that this attribute is mutually exclusive with. (How do you 
      // mark the end of the array?)
      .exclude = NULL
   };
   static struct attribute_spec bitpack_bitcount = {
      .name = "lu_bitpack_bitcount",
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = &attribute_handlers::bitpack_bitcount,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_funcs = {
      .name = "lu_bitpack_funcs",
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = &attribute_handlers::generic_type_or_decl,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_inherit = {
      .name = "lu_bitpack_inherit",
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = &attribute_handlers::generic_type_or_decl,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_range = {
      .name = "lu_bitpack_range",
      .min_length = 2, // min argcount
      .max_length = 2, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = &attribute_handlers::bitpack_range,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_string = {
      .name = "lu_bitpack_string",
      .min_length = 0, // min argcount
      .max_length = 1, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = &attribute_handlers::generic_type_or_decl,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_omit = {
      .name = "lu_bitpack_omit",
      .min_length = 0, // min argcount
      .max_length = 0, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = &attribute_handlers::generic_type_or_decl,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_as_opaque_buffer = {
      .name = "lu_bitpack_as_opaque_buffer",
      .min_length = 0, // min argcount
      .max_length = 0, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler = &attribute_handlers::generic_type_or_decl,
      .exclude = NULL
   };
}

static void register_attributes(void* event_data, void* data) {
   register_attribute(&_attributes::test_attribute);
   register_attribute(&_attributes::bitpack_bitcount);
   register_attribute(&_attributes::bitpack_funcs);
   register_attribute(&_attributes::bitpack_inherit);
   register_attribute(&_attributes::bitpack_range);
   register_attribute(&_attributes::bitpack_string);
   register_attribute(&_attributes::bitpack_omit);
   register_attribute(&_attributes::bitpack_as_opaque_buffer);
}

#include "pragma_handlers/debug_dump_function.h"
#include "pragma_handlers/debug_dump_identifier.h"
#include "pragma_handlers/generate_functions.h"
#include "pragma_handlers/heritable.h"
#include "pragma_handlers/set_options.h"

static void register_pragmas(void* event_data, void* data) {
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "set_options",
      &pragma_handlers::set_options
   );
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "heritable",
      &pragma_handlers::heritable
   );
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "generate_functions",
      &pragma_handlers::generate_functions
   );
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "debug_dump_function",
      &pragma_handlers::debug_dump_function
   );
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "debug_dump_identifier",
      &pragma_handlers::debug_dump_identifier
   );
}

#include "basic_global_state.h"

int plugin_init (
   struct plugin_name_args*   plugin_info,
   struct plugin_gcc_version* version
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
   
   for(int i = 0; i < plugin_info->argc; ++i) {
      const auto& arg = plugin_info->argv[i];
      if (std::string_view(arg.key) == "xml-out") {
         auto& dst = basic_global_state::get().xml_output_path;
         dst = arg.value;
         if (dst.ends_with(".xml")) {
            std::cerr << "lu-bitpack: XML output path set:\n   " << arg.value << "\n";
         } else {
            std::cerr << "lu-bitpack: invalid XML output path (" << arg.value << "); must end in .xml; rejected\n";
         }
         continue;
      }
   }
   
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