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

#include "attribute_handlers/bitpack_as_opaque_buffer.h"
#include "attribute_handlers/bitpack_bitcount.h"
#include "attribute_handlers/bitpack_default_value.h"
#include "attribute_handlers/bitpack_range.h"
#include "attribute_handlers/bitpack_string.h"
#include "attribute_handlers/bitpack_transforms.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "attribute_handlers/generic_type_or_decl.h"
#include "attribute_handlers/no_op.h"
#include "attribute_handlers/test.h"

namespace _attributes {
   static struct attribute_spec test_attribute = {
      
      // attribute name sans leading or trailing underscores; or NULL to mark 
      // the end of a table of attributes.
      .name = "lu_test_attribute",
      
      .min_length =  0, // min argcount
      .max_length = -1, // max argcount (use -1 for no max)
      .decl_required = false, // if true, applying the attr to a type means it's ignored and warns
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = false,
      .handler    = &attribute_handlers::test,
      
      // Can be an array of `attribute_spec::exclusions` objects, which describe 
      // attributes that this attribute is mutually exclusive with. (How do you 
      // mark the end of the array?)
      .exclude = NULL
   };
   
   static struct attribute_spec nonstring = {
      .name = "lu_nonstring",
      .min_length = 0, // min argcount
      .max_length = 0, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::generic_type_or_decl,
      .exclude = NULL
   };
   
   // internal use
   static struct attribute_spec internal_invalid = {
      .name = "lu bitpack invalid attributes",
      .min_length = 0, // min argcount
      .max_length = 0, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::no_op,
      .exclude = NULL
   };
   static struct attribute_spec internal_invalid_by_name = {
      .name = "lu bitpack invalid attribute name",
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::no_op,
      .exclude = NULL
   };
   
   static struct attribute_spec bitpack_as_opaque_buffer = {
      .name = "lu_bitpack_as_opaque_buffer",
      .min_length = 0, // min argcount
      .max_length = 0, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::bitpack_as_opaque_buffer,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_bitcount = {
      .name = "lu_bitpack_bitcount",
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::bitpack_bitcount,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_default_value = {
      .name = "lu_bitpack_default_value",
      .min_length = 1, // min argcount
      .max_length = 1, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::bitpack_default_value,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_range = {
      .name = "lu_bitpack_range",
      .min_length = 2, // min argcount
      .max_length = 2, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::bitpack_range,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_string = {
      .name = "lu_bitpack_string",
      .min_length = 0, // min argcount
      .max_length = 0, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::bitpack_string,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_transforms = {
      .name = "lu_bitpack_transforms",
      .min_length = 1, // min argcount
      .max_length = 2, // max argcount // 2, for internal use
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::bitpack_transforms,
      .exclude = NULL
   };
   static struct attribute_spec bitpack_omit = {
      .name = "lu_bitpack_omit",
      .min_length = 0, // min argcount
      .max_length = 0, // max argcount
      .decl_required = false,
      .type_required = false,
      .function_type_required = false,
      .affects_type_identity  = true,
      .handler = &attribute_handlers::generic_bitpacking_data_option,
      .exclude = NULL
   };
}

static void register_attributes(void* event_data, void* user_data) {
   register_attribute(&_attributes::test_attribute);
   register_attribute(&_attributes::nonstring);
   register_attribute(&_attributes::internal_invalid);
   register_attribute(&_attributes::internal_invalid_by_name);
   register_attribute(&_attributes::bitpack_as_opaque_buffer);
   register_attribute(&_attributes::bitpack_bitcount);
   register_attribute(&_attributes::bitpack_default_value);
   register_attribute(&_attributes::bitpack_range);
   register_attribute(&_attributes::bitpack_string);
   register_attribute(&_attributes::bitpack_transforms);
   register_attribute(&_attributes::bitpack_omit);
}

#include "pragma_handlers/debug_dump_function.h"
#include "pragma_handlers/debug_dump_identifier.h"
#include "pragma_handlers/enable.h"
#include "pragma_handlers/generate_functions.h"
#include "pragma_handlers/set_options.h"

static void register_pragmas(void* event_data, void* user_data) {
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
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "enable",
      &pragma_handlers::enable
   );
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "generate_functions",
      &pragma_handlers::generate_functions
   );
   c_register_pragma_with_expansion(
      "lu_bitpack",
      "set_options",
      &pragma_handlers::set_options
   );
}

#include "basic_global_state.h"

#include "bitpacking/verify_bitpack_attributes.h"

static void on_decl_finished(void* event_data, void* user_data) {
   auto node = (tree) event_data;
   if (node == NULL_TREE)
      return;
   
   if (!basic_global_state::get().enabled)
      return;
   
   // Some bitpacking attributes can only be verified when a DECL is finished. 
   // For example, a C bitfield may have transform options set on its type(s), 
   // and if so, we need to error.
   bitpacking::verify_bitpack_attributes(node);
}
static void on_type_finished(void* event_data, void* user_data) {
   auto node = (tree) event_data;
   if (node == NULL_TREE)
      return;
   
   if (!basic_global_state::get().enabled)
      return;
   
   // Some bitpacking attributes can only be verified when a DECL is finished. 
   // For example, a C bitfield may have transform options set on its type(s), 
   // and if so, we need to error.
   bitpacking::verify_bitpack_attributes(node);
}

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
   register_callback(
      plugin_info->base_name,
      PLUGIN_FINISH_DECL,
      on_decl_finished,
      NULL
   );
   register_callback(
      plugin_info->base_name,
      PLUGIN_FINISH_TYPE,
      on_type_finished,
      NULL
   );
   
   return 0;
}