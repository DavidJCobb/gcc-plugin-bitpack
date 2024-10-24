#include <iostream>

// Must include gcc-plugin before any other GCC headers.
#include <gcc-plugin.h>
#include <plugin-version.h>

// Required: assert that we're licensed under GPL, or GCC will refuse to run us.
int plugin_is_GPL_compatible;

#include <tree.h>

// not allowed to be const
static plugin_info _my_plugin_info = {
   .version = "(unversioned)",
   .help    = "there is no help",
};

#include "handle_struct_type.h"

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
   
   register_callback(
      plugin_info->base_name,
      PLUGIN_INFO,
      /* callback  */ NULL,
      /* user_data */ &_my_plugin_info
   );
   
   std::cerr << "Loaded plug-in: " << plugin_info->base_name << ".\n";
   
   register_callback(
      plugin_info->base_name,
      PLUGIN_FINISH_TYPE,
      plugin_finish_type,
      NULL
   );
   register_callback(
      plugin_info->base_name,
      PLUGIN_ATTRIBUTES,
      register_attributes,
      NULL
   );
   
   return 0;
}