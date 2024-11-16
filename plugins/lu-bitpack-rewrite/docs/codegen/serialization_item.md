# `serialization_item`

A serialization item represents an object or subobject that should be present in the bitstream: we should generate code to read it and write it. They are best conceptualized as a kind of path. When a serialization item refers to a struct, union, or array, it can be expanded to produce a list of serialization items representing the members or elements.

The basic model of code generation is as follows:

1. Loop over all top-level identifiers, which we should allow to be `VAR_DECL`s of any type.
2. Generate "serialization items" wrapping each.
3. Loop over all serialization items, checking against the sector size and bits remaining. If a serialization item doesn't fit, then expand it and try again &mdash; recursively expanding items until they fit.
4. Once we've got one flat list of serialization items per sector, generate the code from those. (See the documentation for re-chunked items and instruction tree nodes.)

Serialization items tend to look like the following:

* `sFoo|a|union_tag`
* `sFoo|a|union_data|(sFoo.a.tag == 0) value_0`
* `sFoo|a|union_data|(sFoo.a.tag == 1) value_1`
* `sFoo|b[5]`
* `sFoo|b[5:8]`
* `sFoo|c as transformed_type|d`
* `sFoo|e as transformed_container_type|f as transformed_member_type`
* `sFoo|a|union_data|(sFoo.a.tag == 1) zero_pad_bitcount(5)`

## Components of a serialization item

A `basic_segment` consists of a `decl_descriptor`, and zero or more `array_access_info`s. Each `array_access_info` represents an array-access operator e.g. `foo[5]`.[^slice]

[^slice]:

    An `array_access_info` actually consists of a start index and a count, and as such may refer to an array slice: `foo[3:7]`. This is used to represent a for-loop over a group of elements wherein each element is handled individually. Thus, if `foo[3]` is not an array, then `foo[0:7]` is also not an array; *but* a serialization item for `foo[0:7]` will report its size in bits as equal to the size in bits of `foo[0]` times 7.

A `padding_segment` consists of a bitcount: the number of zero bits to generate when saving a bitstream, and the numebr of bits to skip when reading a bitstream.

A full `segment` consists of either a `basic_segment` or a `padding_segment`, paired with an optional `condition`. Conditions and zero-padding are used to represent serialization of a union and its contents: we branch on the union tag, accessing different union members conditionally. Conditions are stored per-segment because unions may be nested within each other or within transforms defined by a `decl_descriptor`.

## Expansion

If a serialization item refers to an array, then it expands to a list of serialization items for the array's elements. Expansion never produces an array slice item.

If a serialization item refers to a struct, then it expands to a list of serialization items for the struct's members.

### Expansion of unions

Consider this externally tagged union:

```c
struct Foo {
   int tag;
   union {
     uint16_t a; // ID 0
      float    b; // ID 1
      struct {
         float data;
      } c; // ID 2
   } bar;
} sFoo;
```

A serialization item for `sFoo.bar` should expand to:

* `sFoo|tag`
* `sFoo|bar|(sFoo.tag == 0) a`
* `sFoo|bar|(sFoo.tag == 0) zero_pad_bitcount(16)`
* `sFoo|bar|(sFoo.tag == 1) b`
* `sFoo|bar|(sFoo.tag == 2) c`

This is very similar to how structs expand, except for the added conditions, and the padding to keep all union permutations the same length. During expansion, we'd ignore any member that is omitted and lacks a tagged ID. (If it's omitted, has a tagged ID, and has a default value, then we'd include it. If it's omitted, has a tagged ID, and has no default value, then we'd error.)

Consider this internally tagged union:

```c
union Foo {
   struct {
      int      header;
      int      tag;
      uint16_t data;
      uint8_t  addendum;
   } a; // ID 0
   struct {
      int   header;
      int   tag;
      float data;
      float climate;
   } b; // ID 1
} sFoo;
```

A serialization item for `sFoo` should expand to:

* `sFoo|a|header`
* `sFoo|a|tag`
* `sFoo|(sFoo.a.tag == 0) a|data`
* `sFoo|(sFoo.a.tag == 0) a|addendum`
* `sFoo|(sFoo.a.tag == 0) a|zero_pad_bitcount(40)`
* `sFoo|(sFoo.a.tag == 1) b|data`
* `sFoo|(sFoo.a.tag == 1) b|climate`

This is similar to the above, except that the union's members are expanded during the same step. This has to happen, since the union tag (and any common data preceding it) is part of those members: we can't read them whole because in order to even know *which* we'd *want* to read in whole, we've already read it in part.