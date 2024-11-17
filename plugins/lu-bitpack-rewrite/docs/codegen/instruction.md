
# `codegen::instruction::base`

For code generation, we take the list of top-level variables to serialize and run it through multiple passes before we finally generate code:

* We produce a list of serialization items, which is the easiest medium with which to allot top-level variables into multiple sectors.
* We convert that list into a list of re-chunked items, to make it easier to produce a tree of nodes.
* Finally, we convert those re-chunked items into a tree of `codegen::instruction` nodes, which map almost directly to the code blocks and instructions that we'll be generating.

A `codegen::instruction` node will typically either refer to values to serialize (via a `value_path`), or contain other instruction nodes.

## Types

### `single`

These nodes represent a single value to serialize. This value, described by a `value_path`, may be a single array element, an entire struct, an opaque buffer, a string, an integral value, et cetera. In other words, this and `padding` are the only node types that directly produce code to *serialize a thing.* All other nodes are structural.

### `padding`

These nodes represent zero-padding, used to keep all permutations of a union the same size in the binary output.

### `array_slice`

These nodes represent a `for` loop that we need to generate. They spawn two `VAR_DECL`s &mdash; local variables for the loop counter, in the to-be-generated "read" and "save" functions &mdash; and wrap them in a pair of `decl_descriptor`s. These nodes also contain child nodes, which will actually be responsible for serializing... whatever goes in this loop.

These nodes should never be generated with a count of 1; the algorithm for converting re-chunked items to instruction node trees checks this.

### `transform`

These nodes represent a type transformation to be performed on a value. The generated code for that would look like this:

```c
{
   TransformedType __transformed_value;
   TransformFunction(&untransformed_value, &__transformed_value);
   //
   // Child instruction nodes generate here.
}
```

As such, these nodes spawn two `VAR_DECLs` &mdash; local variables for the final[^transitive-transforms] transformed type; one for the to-be-generated "read" function, and one for the "save" function &mdash; and wrap them in a pair of `decl_descriptors`. These nodes also contain child instructions, which direct codegen to serialize the transformed object or nested fields thereof.

[^transitive-transforms]:

    Transforms may apply transitively. For example, a `FIELD_DECL` may ask to be transformed to type `T`, and type `T` itself may ask to be transformed to type `U`, such that the type that acutally ends up in the bitstream is the serializd form of a `U` instance.
    
    The generated code will need local variables for each type we transform through, i.e. `T __transformed_intermediate` and `U __transformed_value`. However, we only pre-generate the latter &mdash; that is, the node itself only pre-creates variables for the final transformed type &mdash; because those are what member accesses by child nodes would need be relative to; those are the variables that `value_path`s in child nodes would need a way to refer to.

Note that the `transform` node represents only the transformation itself, not serialization of the transformed value (or any members thereof). There should always be at least one child instruction. For example, if the transformed type is a struct, and the entire struct fits in a given sector, then there should be a child `single` node that serializes `__transformed_value`.

### `union_switch` and `union_case`

These nodes represent the branching needed to serialize a tagged union.

A `union_switch` node refers (via a `value_path`) to the tag value to branch on, and stores a map of tag values to `union_case` nodes. A `union_case` node is a generic node for containing other instruction nodes.