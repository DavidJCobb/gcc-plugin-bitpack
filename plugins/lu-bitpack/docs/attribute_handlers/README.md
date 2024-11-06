
# GCC attribute handlers

Attribute handlers are invoked whenever an attribute *may* apply to a declaration or type. They have the following signature:

```c
tree handler(tree* node_ptr, tree name, tree args, int flags, bool* no_add_attrs);
```

If an attribute doesn't use the flags on its defining `attribute_spec` to limit where it can be applied, then given some code like `[[foo]] struct Type bar;`, the `foo` attribute will first be given the opportunity to be applied to `Type`; if it declines that opportunity, it will then be given an opportunity to be applied to `bar`.

You can create arguments that exist only for internal use by putting a space in their names, such that they can never be used in source code.


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

#### List of known flags

As of this writing, the following flags exist:

| Flag | Description |
| :- | :- |
| `ATTR_FLAG_DECL_NEXT` | `node_ptr` points to the `TYPE` of a `DECL`. If your attribute should be applied to the `DECL` rather than the `TYPE`, then return a new attribute node. |
| `ATTR_FLAG_FUNCTION_NEXT` | `node_ptr` points to the return `TYPE` of a function. If your attribute should be applied to the function `TYPE` instead, then return a new attribute node. |
| `ATTR_FLAG_ARRAY_NEXT` | `node_ptr` points to the (innermost) value `TYPE` of a (potentially nested) array (i.e. `uint8_t` given `uint8_t foo[4][3][2]`). If your attribute should be applied to the array `TYPE` instead, then return a new attribute node. |
| `ATTR_FLAG_TYPE_IN_PLACE` | The `node_ptr` points to a `RECORD_TYPE`, `UNION_TYPE`, or `ENUMERATION_TYPE` being created, so you should perform any modifications in place. (If this flag is absent and `node_ptr` is a type, then you should instead modify a copy and then overwrite `node_ptr`.) |
| `ATTR_FLAG_BUILT_IN` | The attribute is being applied by default to a library function whose name indicates known behavior, and should be silently ignored if it's not compatible with the function type. |
| `ATTR_FLAG_CXX11` | The attribute has been parsed as a C++11 attribute (i.e. `[[foo::bar]]` rather than `__attribute__((foo))`). |
| `ATTR_FLAG_INTERNAL` | The attribute handler is being invoked with an internal argument that may not otherwise be valid when specified in source code. |

The `ATTR_FLAG_INTERNAL` flag is used in the following places:

* When parsing the start of a function declaration or definition (`start_function` in `c/c-decl.cc`). If any function arguments have the internal `arg spec` attribute, then these are coalesced into a single `access` attribute that is applied to the function. `ATTR_FLAG_INTERNAL` is used when applying it.
* The handler for the `copy` attribute uses `ATTR_FLAG_INTERNAL` when copying attributes from a source `DECL` or `TYPE` to a destination `DECL`, with the flag used because "the attributes' arguments may be in their internal form."
* The handler for the `malloc` attribute uses `ATTR_FLAG_INTERNAL` to detect and abort recursive invocations.
* The handler for the `access` attribute executes recursively to replace the "external" form of the attribute with a condensed "internal" form, and marks recursive calls with `ATTR_FLAG_INTERNAL`.

### `no_add_attrs`

An out-variable. If you set this value to true, then your attribute will not be added to the `DECL_ATTRIBUTES` or `TYPE_ATTRIBUTES` of the target node. Do this in the event of an error or if your attribute should be applied to some later node.


## Return value

Return `NULL_TREE` if the attribute should be applied only to the current node, or if it should be discarded entirely.

Return `error_mark_node` to cause GCC to issue a warning that the attribute has been ignored. This additionally has the same effect as setting `*no_add_attrs` to `true` and returning `NULL_TREE`.

If the attribute should be applied at a later stage (for example, if you've been invoked on a `*_TYPE`, but you see `ATTR_FLAG_DECL_NEXT` and you want to apply to the `*_DECL`), then return `tree_cons(name, args, NULL_TREE)`, a new attribute node.


## Implementation notes

* If your attribute definition is marked as only being applicable to types (`attribute_spec::type_required`), and after your handler runs, `node_ptr[0]` is a `VAR_DECL`, `PARM_DECL`, or `RESULT_DECL`, then `relayout_decl` is called on the node.

* If an attribute is used multiple times on the same node but with the same arguments, then only one instance of the attribute will exist (presuming the handler doesn't reject all of them via `*no_add_attrs = false`). However, if an attribute is used multiple times on the same node but with different arguments, then multiple instances of the attribute may come to exist on the node (presuming none are rejected as a result of `*no_add_attrs = false`).
  * Attributes are compared by using `simple_cst_equal` on their arguments. If any argument is not a constant (or if otherwise-equal constant are wrapped in non-equal `EXPR_LOCATION` nodes), then the attributes compare as non-equal.


## Common tasks

### Automatically adding another attribute

To add an attribute to a given node, whether from an attribute handler or generally, call:

```c
auto attr_name = get_identifier("attr_name");
auto attr_args = NULL_TREE;
auto attr_next = NULL_TREE;

auto attr_node = tree_cons(attr_name, attr_args, attr_next);

int flags = 0;

decl_attributes(&target_node, attr_node, flags, nullptr);
```

If you need to add multiple attributes at a time, then connect them via a `TREE_CHAIN`. The third argument to the `tree_cons` function (`attr_next` in our code snippet) is a chain node to prepend to (such that the newly-created `attr_node` is the head of the chain).