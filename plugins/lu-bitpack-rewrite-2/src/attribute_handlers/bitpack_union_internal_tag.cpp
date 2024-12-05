#include "attribute_handlers/bitpack_union_internal_tag.h"
#include "attribute_handlers/helpers/bp_attr_context.h"
#include "attribute_handlers/generic_bitpacking_data_option.h"
#include "gcc_wrappers/constant/string.h"
#include "gcc_wrappers/type/array.h"
#include "gcc_wrappers/type/untagged_union.h"
#include "gcc_wrappers/identifier.h"
#include <diagnostic.h>
#include "bitpacking/verify_union_internal_tag.h"
#include "bitpacking/verify_union_members.h"
namespace gw {
   using namespace gcc_wrappers;
}

namespace attribute_handlers {
   extern tree bitpack_union_internal_tag(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs) {
      auto result = generic_bitpacking_data_option(node_ptr, name, args, flags, no_add_attrs);
      if (*no_add_attrs) {
         return result;
      }
      
      if (flags & ATTR_FLAG_INTERNAL) {
         return NULL_TREE;
      }
      helpers::bp_attr_context context(node_ptr, name, flags);
      
      gw::type::optional_untagged_union union_type;
      {
         auto type = context.type_of_target();
         while (type.is_array())
            type = type.as_array().value_type();
         if (type.is_union())
            union_type = type.as_union();
      }
      if (!union_type) {
         context.report_error("can only be applied to unions");
      }
      
      gw::constant::optional_string data;
      {
         auto next = TREE_VALUE(args);
         if (next != NULL_TREE && gw::constant::string::raw_node_is(next)) {
            data = next;
         } else {
            context.report_error("argument must be a string constant");
         }
      }
      
      *no_add_attrs = true;
      if (data) {
         auto view = data->value();
         if (view.empty()) {
            context.report_error("tag identifier cannot be blank");
         }
         if (TREE_CODE(node_ptr[0]) == TYPE_DECL) {
            //
            // For `[attr] typedef union { ... } foo`, the attributes apply to 
            // the typedef, not to the union type itself. At the time that the 
            // PLUGIN_FINISH_TYPE callback fires, nothing exists to link the 
            // union type to the still-being-spawned typedef, so the validation 
            // we do in PLUGIN_FINISH_TYPE won't have any effect (it won't see 
            // anything to validate).
            //
            // Fortunately, the type should be complete here, so we can validate 
            // it now.
            //
            // Checking these unions for errors is a goddamned mess.
            //
            if (union_type) {
               bool success =
                  bitpacking::verify_union_members(*union_type) &&
                  bitpacking::verify_union_internal_tag(*union_type, view.data(), false)
               ;
               if (!success)
                  context.report_error("encountered errors (see previous message(s))");
            }
         } else {
            //
            // We can't validate the contents of the union type, because attribute 
            // handlers applied to types always see the types as incomplete. (If 
            // they're applied to typedefs, that's a different story, but we need 
            // to support all ways of defining a union.) We can report errors from 
            // PLUGIN_FINISH_TYPE, but can't apply attributes to mark the types as 
            // invalid.
            //
         }
      }
      
      *no_add_attrs = true;
      if (!context.has_any_errors()) {
         assert(!!data);
         auto view = data->value();
         
         gw::list_node args({}, gw::identifier(view.data()));
         context.reapply_with_new_args(args);
      }
      return NULL_TREE;
   }
}