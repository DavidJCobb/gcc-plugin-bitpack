#include <iostream>

// Must include gcc-plugin before any other GCC headers.
#include <gcc-plugin.h>
#include <plugin-version.h>

// Required: assert that we're licensed under GPL, or GCC will refuse to run us.
int plugin_is_GPL_compatible;

#include <tree.h>

#include "handle_struct_type.h"

static void plugin_finish_type(void *event_data, void *user_data) {
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
}

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
   
   std::cerr << "Loaded plug-in: " << plugin_info->base_name << ".\n";
   
   register_callback(
      plugin_info->base_name,
      PLUGIN_FINISH_TYPE,
      plugin_finish_type,
      NULL
   );
   
   return 0;
}