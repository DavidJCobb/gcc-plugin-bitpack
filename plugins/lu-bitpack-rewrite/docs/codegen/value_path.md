
# `codegen::value_path`

A `codegen::value_path` represents a path to a value, for `codegen::instruction` nodes.

Throughout the code generation process, we need a way to refer to potentially nested objects within arrays, structs, and unions. Serialization items rely on `basic_segment`s to keep track of this information, each segment referring to a single `decl_descriptor` and zero or more array accesses (encoded as an integer start index and an integer count). Re-chunked items rely on "qualified decl" chunks (each holding a list of `decl_descriptor` pointers) for member access and "array slice" chunks for array access (again using integers).

As we get closer to code generation, however, it becomes easiest to work purely in terms of `DECL` nodes in GCC and `decl_descriptors` which wrap them for our convenience. In particular, for situations where we need to generate a `for` loop, we want a `codegen::instruction` node for the `for` loop itself, which creates and owns a pair[^pairs] of `VAR_DECL`s for the loop counter, wrapping them in `decl_descriptor`s; and then we want the nodes therein to refer to that loop counter. Put more briefly: where serialization items encode `foo[0:5]`, we want `codegen::instruction` nodes to encode `foo[__i]`, where the `for` loop node that owns `__i` knows that its bounds should be [0, 5).

[^pairs]: We need to generate "read" and "save" functions; ergo we need separate local variables for "read" and "save" code; ergo anything that would refer to a `decl_descriptor` must in fact refer to two, though they can be the same `decl_descriptor` if they're a `FIELD_DECL` or a non-local `VAR_DECL`.

Another example is type transformations, which need to act on the local variables that are of the transformed type. If we're serializing `a|b as new_type|c`, then we need the `codegen::instruction` node that serializes `c` to encode `__transformed_b.c`, not `a.b.c`, as `a.b.c` &mdash; a `c` field on the original, non-transformed `b` value &mdash; may not even exist.

For this reason, `codegen::instruction` nodes use `value_path` objects. Each path is a set of segments; a segment may be a pair[^pairs] of `decl_descriptors` for top-level values or member access, a pair for array access using a loop counter, or an integer constant for when we're accessing a specific single array element.