
# Re-chunked items

We use <dfn>serialization items</dfn> to properly divide to-be-serialized values across multiple sectors: each sector gets its own flat list of serialization items, each item acting as a path to a (potentially nested) to-be-serialized value. However, a flat list of serialization items isn't an ideal thing to base code generation on, because we'll generally want to generate nested blocks of code. It would be easier to generate a tree structure from the serialization item list, and then do code generation based on that tree.

It's easier still, however, to convert the flat list of serialization items into a flat list of "re-chunked" items that are easier to then process into a tree of "instructions."

## Rationale

Consider the following example:

```c
static struct Foo {
   int tag;
   union {
      int a;
      int b;
      int c;
      struct {
         int tag;
         union {
            int x;
            int y;
         } data;
      } d;
   } data;
} sFoo;
```

The `sFoo` variable would produce this flat list of serialization items:

```
sFoo|tag
sFoo|data|(sFoo.tag == 0) a
sFoo|data|(sFoo.tag == 0) zero_pad_bitcount(96)
sFoo|data|(sFoo.tag == 1) b
sFoo|data|(sFoo.tag == 1) zero_pad_bitcount(96)
sFoo|data|(sFoo.tag == 2) c
sFoo|data|(sFoo.tag == 2) zero_pad_bitcount(96)
sFoo|data|(sFoo.tag == 3) d|tag
sFoo|data|(sFoo.tag == 3) d|data|(sFoo.data.d.tag == 0) x
sFoo|data|(sFoo.tag == 3) d|data|(sFoo.data.d.tag == 1) y
```

From those items, we'd somehow have to produce code vaguely resembling the following. (The actual function calls and whatnot would differ, but this serves to illustate the general structure.)

```c
void __example() {
   __stream(sFoo.tag);
   if (sFoo.tag == 0) {
      __stream(sFoo.data.a);
      __zero_pad(96);
   } else if (sFoo.tag == 1) {
      __stream(sFoo.data.b);
      __zero_pad(96);
   } else if (sFoo.tag == 2) {
      __stream(sFoo.data.c);
      __zero_pad(96);
   } else if (sFoo.tag == 3) {
      __stream(sFoo.data.d.tag);
      if (sFoo.data.d.tag == 0) {
         __stream(sFoo.data.d.data.x);
      } else if (sFoo.data.d.tag == 1) {
         __stream(sFoo.data.d.data.y);
      }
   }
}
```

Among other things, we have to group accesses that occur under the same conditions, as well as accesses to fields of transformed values. Doing that *and* codegen at the same time would be messy, and so we transform serialization items into re-chunked items, and re-chunked items into instruction nodes.

## What is a re-chunked item?

A re-chunked item is a serialization item that has been divided apart at the points where we'd generate parent and child nodes. Where serialization items are divided into segments, re-chunked items are divided into chunks:

* **Qualified decl** chunks represent a root variable and/or member access: `foo|bar|baz` becomes a single qualified decl chunk with three `decl_descriptor` pointers.
* **Condition** chunks represent a condition that must pass: where serialization items attach conditions to segments, re-chunked items give conditions their own entire chunk.
  * The LHS value is itself identified via a list of (non-condition) chunks.
* **Array slice** chunks represent a single rank of array access: `foo[3][1:4]` becomes a qualified decl chunk followed by two array slice chunks. When we generate a tree of instruction nodes, an array slice chunk that spans more than one element will produce an array slice node, representing a `for` loop.
* **Transform** chunks represent type transformations requested by the `lu_bitpack_transforms` attribute. When transformations apply transitively (e.g. because a field transforms to one type, and that type transforms to another type), a single chunk is used.
* **Padding** chunks represent zero-padding, used to keep unions a consistent size within the bitstream.

The string representation of a re-chunked item is as follows:

* `\`decl.decl.decl\`` for qualified-decls.
* `(lhs == rhs)` for conditions.
* `[start]` for array-slices that refer to a single element, or `[start:end]` for array-slices that refer to the range [start, end).
* `<as typename as other_typename>` for transforms, where the last type in the list is the type that'll be written to the bitstream.
* `{pad:16}` for zero-padding, listing the bitcount used (e.g. 16).

The above example, then, would produce these re-chunked items:

```
`sFoo.tag`
`sFoo.data` (`sFoo.tag` == 0) `a`
`sFoo.data` (`sFoo.tag` == 0) {pad:96}
`sFoo.data` (`sFoo.tag` == 1) `b`
`sFoo.data` (`sFoo.tag` == 1) {pad:96}
`sFoo.data` (`sFoo.tag` == 2) `c`
`sFoo.data` (`sFoo.tag` == 2) {pad:96}
`sFoo.data` (`sFoo.tag` == 3) `d.tag`
`sFoo.data` (`sFoo.tag` == 3) `d.data` (`sFoo.data.d.tag` == 0) `x`
`sFoo.data` (`sFoo.tag` == 3) `d.data` (`sFoo.data.d.tag` == 0) `y`
```

When it comes time to create a tree of instructions, we can now maintain a stack that maps chunks almost directly to hierarchical nodes, where the depth of a chunk almost exactly corresponds to the nesting level of the node it produced. This makes it very easy to tell whether the next re-chunked item should be located within the same hierarchy as the previous re-chunked item.
