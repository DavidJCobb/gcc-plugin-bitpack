# `serialization_item`

A serialization item represents an object or subobject that should be present in the bitstream: we should generate code to read it and write it. They are best conceptualized as a kind of path. When a serialization item refers to a struct, union, or array, it can be expanded to produce a list of serialization items representing the members or elements.

The basic model of code generation is as follows:

1. Loop over all top-level identifiers, which we should allow to be `VAR_DECL`s of any type.
2. Generate "serialization items" wrapping each.
3. Loop over all serialization items, checking against the sector size and bits remaining. If a serialization item doesn't fit, then expand it and try again &mdash; recursively expanding items until they fit.
4. Once we've got one flat list of serialization items per sector, generate the code from those.

Serialization items tend to look like the following:

* `sFoo.a.tag`
* `if (sFoo.a.tag == 0) sFoo.a.data`
* `sFoo.b[5]`
* `sFoo.b[5:8]`
* `((transformed_type) sFoo.c).d`
* `((transformed_member_type) ((transformed_container_type) sFoo.e).f)`
* `if (sFoo.a.tag == 1) zero_pad_bitcount(5)`

## Components of a serialization item

Serialization items typically consist of conditions and segments. Some items are used to represent zero padding, and as such consist only of a segment and a padding size.

Each segment consists of:

* A `decl_descriptor` pointer.
* Zero or more `array_access_info`s. Each represents an array-access operator (i.e. `foo[5]`).[^slice]

[^slice]:

    As a convenience for the sector-splitting code, an `array_access_info` consists of a start index and a count, and as such may refer to an array slice: `foo[3:7]`. This is used to represent a for-loop over a group of elements wherein each element is handled individually. Thus, if `foo[3]` is not an array, then `foo[0:7]` is also not an array; *but* the size in bits of `foo[0:7]` *is* considered equal to the size in bits of `foo[0]` times 7.

Conditions are used to represent serialization of a union and its contents: we branch on the union tag, accessing different union members conditionally. A serialization item may have multiple conditions in order to represent the case of nested unions; and the conditions are AND-linked. Each condition consists of:

* A list of segments representing a simplified path (e.g. to a union tag).
* A value to which the referred-to value must compare equal.

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

* `if (sFoo.tag == 0) sFoo.bar.a`
* `if (sFoo.tag == 0) zero_pad_bitcount(16)`
* `if (sFoo.tag == 1) sFoo.bar.b`
* `if (sFoo.tag == 2) sFoo.bar.c`

This is very similar to how structs expand, except for the added conditions, and the padding to keep all union permutations the same length. During expansion, we'd ignore any member that is omitted and lacks a tagged ID. (If it's omitted, has a tagged ID, and has a default value, then we'd include it. If it's omitted, has a tagged ID, and has no default value, then we'd error.)

We currently have no way of representing zero-padding via a `serialization_item`. Remedying that would be the easiest way to get zero-padding implemented.

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

* `sFoo.a.header`
* `sFoo.a.tag`
* `if (sFoo.a.tag == 0) sFoo.a.data`
* `if (sFoo.a.tag == 0) sFoo.a.addendum`
* `if (sFoo.a.tag == 0) zero_pad_bitcount(40)`
* `if (sFoo.a.tag == 1) sFoo.b.data`
* `if (sFoo.a.tag == 1) sFoo.b.climate`

This is similar to the above, except that the union's members are expanded during the same step. This has to happen, since the union tag (and any common data preceding it) is part of those members: we can't read them whole because in order to even know *which* we'd *want* to read in whole, we've already read it in part.