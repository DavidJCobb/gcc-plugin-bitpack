
# GCC attribute handlers

Attribute handlers are invoked whenever an attribute *may* apply to a declaration or type. They have the following signature:

```c
tree handler(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs);
```

If an attribute doesn't use the flags on its defining `attribute_spec` to limit where it can be applied, then given some code like `[[foo]] struct Type bar;`, the `foo` attribute will first be given the opportunity to be applied to `Type`; if it declines that opportunity, it will then be given an opportunity to be applied to `bar`.


## Arguments

### `node_ptr`

A pointer to up to three tree nodes:

* `node_ptr[0]` is the node that this attribute is being given an opportunity to apply to.
* `node_ptr[1]` is the last pushed/merged declaration, if one exists.
* `node_ptr[2]` may be the declaration of `node_ptr[0]`, if `node_ptr[0]` is not itself a declaration.

Note that not all of the node's data will be ready yet. For example, as of this writing, a `FIELD_DECL` only has its `DECL_CONTEXT` set after attributes are applied.

Some attributes are meant to make changes to the nodes they apply to, beyond simply being added to those nodes' attribute lists. If you need to alter a `*_DECL` node, then you should make your changes to the node directly. However, if you need to alter a `*_TYPE` node, then you should instead create a copy of the type, modify the copy, and overwrite `*node_ptr` with the copy.

### `name`

An `IDENTIFIER_NODE` indicating the name of the attribute.

### `args`

A `TREE_LIST` of the attribute's arguments, in the same format as when iterating over already-applied attributes.

### `flags`

A flags mask which contains information about the context in which the attribute was used. The specific values are defined in the `attribute_flags` enum in `tree-core.h`.

Consider the following example:

```c
[[foo]] struct Type;

[[bar]] struct Type variable;
```

When the `bar` handler is invoked for the `Type` type, the `ATTR_FLAG_DECL_NEXT` flag will be present, and this is how you would tell the two situations apart.

### `no_add_attrs`

An out-variable. If you set this value to true, then your attribute will not be added to the `DECL_ATTRIBUTES` or `TYPE_ATTRIBUTES` of the target node. Do this in the event of an error or if your attribute should be applied to some later node.


## Return value

Return `NULL_TREE` if the attribute should be applied only to the current node, or if it should be discarded entirely.

Return `error_mark_node` to cause GCC to issue a warning that the attribute has been ignored. This additionally has the same effect as setting `*no_add_attrs` to `true` and returning `NULL_TREE`.

If the attribute should be applied at a later stage (for example, if you've been invoked on a `*_TYPE`, but you see `ATTR_FLAG_DECL_NEXT` and you want to apply to the `*_DECL`), then return `tree_cons(name, args, NULL_TREE)`, a new attribute node.