
# Attributes

GCC stores attributes as a key/value map. We could use `list_node` to refer to them, but it's friendlier to define dedicated wrappers for them.

Within the map, keys are the attribute name (as an `IDENTIFIER_NODE`) and values are the attribute's arguments, if any. The arguments are themselves encoded as a key/value map, even though only the values are documented as being meaningful.


## Types

### `attribute`

A wrapper around a single attribute node; from here, you can access the attribute's name, namespace[^attr-namespace], or arguments. You can also check whether the argument was parsed as an `[[attribute]]` or an `__attribute__((...))`.

Officially, attribute arguments can be an identifier node on its own, an identifier node followed by `EXPR` nodes, or just `EXPR` nodes. However, tests with the C parser show that at least as of GCC 11.4.0, other node types (e.g. `FUNCTION_DECL` if you use a function's identifier) parse properly.


### `attribute_list`

A collection of attributes present on a `TYPE` or `DECL`. These attributes may not necessarily be in the same order as in source code.[^unordered-attr-list]

[^unordered-attr-list]: Per comments in `decl_attributes` defined in `attribs.cc`: "Note that attributes on the same declaration are not necessarily in the same order as in the source."


## Implementation notes

`attribute_list` is a wrapper for a key/value pair node in the attribute list. Key/value maps are stored as linked lists of pair nodes, so the head node represents the entire map.

`attribute` also wraps a key/value node. This is both so that we can retrieve the attribute's name (since that's stored in or as[^attr-namespace] the key), and so we can tell "no attribute" apart from "attribute with no arguments," since an attribute may have a `NULL_TREE` arguments node.

Attribute arguments are a key/value map (`TREE_LIST`), but only the values are defined. Some GCC internal attributes co-opt the keys (`TREE_PURPOSE`) to store additional data, but I don't know if that's something plug-ins can do with the expectation that it'll still be valid in the future, so I don't expose it.


[^attr-namespace]: Attributes defined with C++11 syntax (`[[foo::bar]]`) can have a namespace in addition to their regular name. This affects how the attribute name is stored internally, which is why you should always use GCC's `get_attribute_name` and `get_attribute_namespace` accessors for attribute nodes instead of [groveling](https://devblogs.microsoft.com/oldnewthing/20031223-00/?p=41373) into the internal representation to retrieve the name.