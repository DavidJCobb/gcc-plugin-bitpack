#include "attribute_handlers/no_op.h"

namespace attribute_handlers {
   extern tree no_op(tree* node, tree name, tree args, int flags, bool* no_add_attrs) {
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
      //    For example: if you define an attribute that doesn't specifically 
      //    indicate that it only belongs on a declaration, or only on a type, 
      //    then upon parsing something like `[[foo]] struct Type field`, GCC 
      //    will first invoke your handler on the `Type` type node with the 
      //    `ATTR_FLAG_DECL_NEXT` flag. If you want the attribute to apply to 
      //    the Type type, then act as normal. However, if you instead want 
      //    the attribute to apply to `field`, then set `no_add_attrs` to true 
      //    and return `*node`, and you should then be called again on `field`.
      //
      // If you set `no_add_attrs` to true, then your attribute will not be 
      // added to the DECL_ATTRIBUTES or TYPE_ATTRIBUTES of the target node. 
      // Do this on error or in any other case where your attribute shouldn't 
      // get added.
      //
      // "Depending on FLAGS, any attributes to be applied to another type or 
      // DECL later may be returned; otherwise the return value should be 
      // NULL_TREE.  This pointer may be NULL if no special handling is 
      // required beyond the checks implied by the rest of this structure." 
      // Quoted verbatim because I have no idea what that means.
      //
      *no_add_attrs = false;
      return NULL_TREE;
   }
}