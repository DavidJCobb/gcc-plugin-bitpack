#include <iostream>
#include <stdexcept>

// Must include gcc-plugin before any other GCC headers.
#include <gcc-plugin.h>
#include <plugin-version.h>

// Required: assert that we're licensed under GPL, or GCC will refuse to run us.
int plugin_is_GPL_compatible;

#include <tree.h>
#include <c-family/c-pragma.h>

// not allowed to be const
static plugin_info _my_plugin_info = {
   .version = "(unversioned)",
   .help    = "there is no help",
};

#include "pragma_handlers/debug_dump_function.h"
#include "pragma_handlers/debug_dump_identifier.h"
#include "pragma_handlers/enable.h"
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
      "set_options",
      &pragma_handlers::set_options
   );
}

#include "gcc_wrappers/events/on_type_finished.h"

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
      PLUGIN_PRAGMAS,
      register_pragmas,
      NULL
   );
   {
      auto& mgr = gcc_wrappers::events::on_type_finished::get();
      mgr.initialize(plugin_info->base_name);
      /*mgr.add(
         "Valid8Ty",
         [](gcc_wrappers::type::base type) {
            if (!basic_global_state::get().enabled)
               return;
            //
            // ...
            //
         }
      );*/
   }
   
   return 0;
}