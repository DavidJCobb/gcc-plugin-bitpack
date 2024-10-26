
# Useful GENERIC functions

## On types

### Struct or union types

#### `lookup_field`

```c
tree field_del_list = lookup_field(type_node, field_identifier_node)
```

## For building expressions

### `COMPONENT_EXPR` (member access)

**Defined in:** `c-tree.h`
```c
#if GCCPLUGIN_VERSION_MAJOR >= 9 && GCCPLUGIN_VERSION_MINOR >= 5
   #if GCCPLUGIN_VERSION_MAJOR >= 13 && GCCPLUGIN_VERSION_MINOR >= 3
      // https://github.com/gcc-mirror/gcc/commit/bb49b6e4f55891d0d8b596845118f40df6ae72a5
      extern tree build_component_ref(
         location_t loc,
         tree datum,
         tree component,
         tree component_loc,
         tree arrow_loc,
         bool handle_counted_by
      );
   #else
      // GCC PR91134
      // https://github.com/gcc-mirror/gcc/commit/7a3ee77a2e33b8b8ad31aea27996ebe92a5c8d83
      extern tree build_component_ref(
         location_t loc,
         tree datum,
         tree component,
         tree component_loc,
         tree arrow_loc
      );
   #endif
#else
   extern tree build_component_ref(
      location_t loc,
      tree datum,
      tree component,
      tree component_loc
   );
#endif
```

Create an expression (`COMPONENT_EXPR`) that refers to the `component` field of the `datum` struct or union. The `component` argument is an `IDENTIFIER_NODE`.

This function properly handles access through anonymous member structs or unions to the named members inside. In such cases, it will return the topmost expression of a set of nested `COMPONENT_EXPR` descending down to the target field.

#### Args

<dl>
   <dt><code>loc</code></dt>
      <dd>The location of the member access expression as a whole.</dd>
   <dt><code>datum</code></dt>
      <dd>A <code>VAR_DECL</code> or similar <code>_DECL</code>: the struct or union value to perform member access on.</dd>
   <dt><code>component</code></dt>
      <dd>An <code>IDENTIFIER_NODE</code> indicating the name of the field to access.</dd>
   <dt><code>component_loc</code></dt>
      <dd>The source code location of <code>component</code>.</dd>
   <dt><code>arrow_loc</code></dt>
      <dd>If this isn't <code>UNKNOWN_LOCATION</code>, then the member access is being performed with the <code>-&gt;</code> operator, and this is the location of the operator.</dd>
   <dt><code>handle_counted_by</code></dt>
      <dd>
         <p>If true, check the <code>counted_by</code> attribute and generate a call to <code>.ACCESS_WITH_SIZE</code>. Otherwise, ignore the attribute.</p>
         <p>The <code>counted_by</code> attribute is used to annotate a flexible array member on a struct, identifying a sibling member that indicates the array's current length.</p>
      </dd>
</dl>


## Misc

### `lookup_attribute`

**Defined in:** `attribs.h`

```c
inline tree lookup_attribute(const char* attr_name, tree list);
```

Given a `TREE_LIST` of attributes and the name (sans leading and trailing double-underscores) of some desired attribute, searches the list for an attribute with said name, and returns it if found. Returns `NULL_TREE` otherwise.

If multiple attributes with the same name are present, this returns only the first. You can feed that first attribute back into the function (i.e. `lookup_attribute("desired_name", found_attr)`) to search for any duplicates.



# Plug-in callbacks

## `PLUGIN_START_PARSE_FUNCTION`

Invoked when parsing of a function begins, before the body has been seen, with a tree passed as an argument. Check if the tree is not `NULL_TREE` and is a `FUNCTION_DECL` before proceeding (if either condition is met, a parse error will be emitted, but only after the plug-in callback is made).

At this point, the following information is available about the function:

* `DECL_EXTERNAL(decl)`
* `DECL_DECLARED_INLINE_P(decl)` (whether the function was declared with the `inline` keyword)
* `DECL_UNINLINABLE(decl)`
* `TREE_PUBLIC(decl)`
  * ...Partially. If this is a nested function, then this flag will have to be forced to 0, but that only happens after the plug-in callback has run.
* `DECL_SOURCE_LOCATION(decl)`
* Declspecs
* Function type (`TREE_TYPE(decl)`)
  * ...Partially. If the return type is not a complete type, then it will need to be forced to `void`, but that only happens after the plug-in callback has run.
* Identifier
* Return type and attributes thereof
  * ...But the `RESULT_DECL` doesn't exist yet.
* Attributes
  * ...Partially. Some "implicit attributes" are added after the plug-in callback has run (see the compiler-internal function `c_decl_attributes`).
  * ...Partially. If `#pragma weak` is used on the function, the effect is applied after the plug-in callback has run.

The following information *may* be available:

* Parameter types, as `TYPE_ARG_TYPES(TREE_TYPE(decl))`
  * If this is not available at the time the plug-in callback runs, and if the function has previously been forward-declared, then the data will be copied from that forward declaration.