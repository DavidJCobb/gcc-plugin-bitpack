
# gcc-plugin-bitpack

An attempt at building a GCC plug-in that can generate code for bitpacking data structures in a C program. Initial development guided heavily by [rofirrim/gcc-plugins](https://github.com/rofirrim/gcc-plugins); makefiles borrowed from there as well. (GCC plug-ins, this one included, are compulsorily GPL'd or GPL-compatible, so it's fine.)

As of this writing, I'm currently targeting GCC 11.4.0.

## Feature goals

* Annotate struct members with information specifying how they should be bitpacked
  * Set a bitcount explicitly or compute one dynamically.
  * Booleans should encode as single-bit by default.
  * Integral fields should be able to compute a bitcount based on their min and max values. Where these are not set, they default to the minimum and maximum representable values in the integral's type.
  * For strings, indicate whether they're null-terminated in memory.
  * Ability to mark types for statistics reporting purposes.
* Automatically generate code to serialize structs to a bitpacked format, and read them back from that format.
  * Optional capacity limits
    * Optionally divide bitpacked data into "sectors" of a limited size. Serialization should pause at the end of one sector &mdash; even if this means serializing only part of a struct or array &mdash; and resume where it left off when beginning the next sector.
    * Sector size
    * Sector count
    * Overall size
  * Export an XML file describing the resulting format.
    * This is meant to aid with structs that may change, by facilitating the development of external tools that (guided by the XML) can read data in an older format and convert it to a newer one.
    * Currently possible via a command line argument: `-fplugin-arg-lu_bitpack-xml-out=$(DESIRED_PATH)/test.xml`
  * Export a human-readable report stating how much space would be consumed by a non-bitpacked representation (i.e. blind `memcpy`ing) versus how much is consumed by the bitpacked representation.
    * This is meant to aid with retrofitting this bitpacking scheme into programs that did not previously use it: use the report to figure out what parts of a struct are more "expensive," and then study them and the code that operates on them in detail, to figure out how you can tighten the bitpacking options for those fields.

## Distant goals

* Some form of GCC version-independence
  * Study GCC commit history and figure out what range of GCC versions should be compatible with the plug-in given the data structures and functions we access?
  * Wrap GCC internals somehow, to facilitate supporting a larger range of versions by adapting differently-versioned data structures to a common interface?
  * What about the ABI?

## Non-goals

* Presence bits (think "`std::optional`"): the ability to pack data using the smallest possible representation. My current use case entails packing data into a limited storage space in such a manner that the largest possible data must fit; ergo bitpacked data must always be uniform in size, so we can check statically whether we have enough room.


## Documentation

### Attributes

Attributes may be applied to non-array types or to struct field declarations in order to configure how those fields are serialized.

The following attributes may be used on non-array types or on struct field declarations:

<dl>
   <dt><code>lu_bitpack_default_value(<var>value</var>)</code></dt>
      <dd>
         <p>Designates a default value for the entity, with respect to bitpacking. The default value will be emitted in the XML output, and if an omitted field (see <code>lu_bitpack_omit</code>) has a default value, then the generated read function will assign that value to that field.</p>
         <p>The following value types are valid:</p>
         <ul>
            <li>Integer constants, if the field is an integer.</li>
            <li>Floating-point constants, if the field is a floating-point number.</li>
            <li>String literals, if the field is an array. The string literal must not be longer than the array. (It may be shorter, in which case excess bytes will be <code>memset</code> to zero.) The presence of GCC's <code>nonstring</code> attribute is detected and accounted for.</li>
         </ul>
      </dd>
   <dt><code>lu_bitpack_inherit("<var>name</var>")</code></dt>
      <dd><p>Indicates that this entity inherits a set of options defined via <code>#pragma lu_bitpack heritable</code>.</p></dd>
   <dt><code>lu_bitpack_omit</code></dt>
      <dd><p>Indicates that this entity should be omitted from bitpacking. If an omitted field has a default value, then the generated "read" function will set the field to that value.</p></dd>
</dl>

The following groups of options are mutually exclusive.

#### Integral options

These options are only permitted on: `typedef`s of boolean, enum, or integer types; `typedef`s of (potentially multi-dimensional) arrays of such types; or fields whose types are booleans, enums, integers, or (potentially multi-dimensional) arrays thereof.

When these attributes are applied to [fields that are of] an array type, the attributes are assumed to pertain to the innermost value type. For example, setting an explicit bitcount of 3 on <code>int field[5]</code> will cause the field's serialized representation to consume 15 bits in total.

<dl>
   <dt><code>lu_bitpack_bitcount(<var>n</var>)</code></dt>
      <dd><p>Sets an explicit size in bits for the field's serialized representation.</p></dd>
   <dt><code>lu_bitpack_range(<var>min</var>, <var>max</var>)</code></dt>
      <dd><p>Indicates the minimum and maximum possible values for the field, influencing the computed size in bits for the field's serialized representation (if that is not set explicitly).</p></dd>
</dl>

If you don't set an explicit bitcount or a range for an integral type or field, then the computed minimum is the minimum representable value in the field's type, and the computed bitcount is <code>std::bit_width((uintmax_t)(max - min))</code>. For example, the default size in bits for the serialized representation of a <code>int8_t</code> is 8, and the serialized representation takes the form <code>v - std::numeric_limits<int8_t>::lowest()</code>.

If you set a range but not a bitcount, then the bitcount is computed from the range. For example, an integral field defined to permit values in the range [-5, 5] will be serialized as <code>v + 5</code> and will have a serialized size of 4 bits.

Booleans are a notable exception. When defining global bitpacking options, you can specify the identifier of an integral typedef. Any field of this type (or of a typedef thereoF) will be considered a boolean: absent any other bitpacking options, it will serialize as a single-bit value by default.

#### Opaque buffer options

When these attributes are applied to fields of an array type, the attributes are assumed to pertain to the innermost value type. That is: we currently use a for-loop to serialize each array element individually, rather than serializing the entire array as a unit.

<dl>
   <dt><code>lu_bitpack_as_opaque_buffer</code></dt>
      <dd><p>Indicates that the field should be serialized as an opaque buffer &mdash; akin to <code>memcpy</code>ing it, were it possible to <code>memcpy</code> to a destination measured in bits rather than bytes.</p></dd>
</dl>

The serialized size of a field of type <code>T</code> encoded as an opaque buffer is <code>sizeof(T)</code> bytes.

#### String options

These options are only permitted on: an array type or multi-dimensional array type whose value type is a single-byte integral type; or fields of such a type.

When these attributes are applied to multi-dimensional arrays, the options are assumed to pertain to the innermost array. For example, applying them to <code>char names[5][10]</code> produces five strings of length 10.

<dl>
   <dt><code>lu_bitpack_string</code></dt>
      <dd><p>Indicates that the field should be serialized as a string.</p></dd>
</dl>

Strings are assumed to require a null terminator by default, such that given a string <code>char field[<var>n</var>]</code>, the max length is <var>n</var> &minus; 1. If a string has GCC's <code>nonstring</code> attribute, however, then the null terminator is considered optional.

Strings are not handled substantially different from opaque buffers, except that they may use different bitstream functions (one for always-terminated strings and one for optionally-terminated strings). These bitstream functions may process the value differently as is necessary (e.g. setting all bytes past the first null terminator to zero, on read).

#### Transformation options

When these attributes are applied, they designate <dfn>transformation functions</dfn> that will be used to convert the value just before its serialized and just after it's deserialized. The value's ordinary type is the <dfn>in situ type</dfn>, whereas the type that is serialized into a bitstream is the <dfn>transformed</dfn> type.

When these attributes are applied to [fields that are of] an array type, the attributes are assumed to pertain to the innermost value type.

<dl>
   <dt><code>lu_bitpack_transforms("pre_pack=<var>pre</var>,post_unpack=<var>post</var>")</code></dt>
      <dd><p>Specify <dfn>pre-pack</dfn> and <dfn>post-unpack</dfn> functions by their identifiers. These functions must exist at file scope.</p>
      <p>The pre-pack function should have the signature <code>void pre(const <var>InSitu</var>*, <var>Transform</var>*)</code>. The post-unpack function should have the signature <code>void post(<var>InSitu</var>*, const <var>Transform</var>*)</code>.</p></dd>
</dl>

#### Union options

Unions can only be serialized if they are internally or externally tagged: you must indicate what object serves as the union's tag, and what values correspond to which union members.

An externally tagged union must exist inside of a struct.[^external-direct-container] The tag may be any previously seen member of that struct (or any member thereof).

[^external-direct-container]:

    The externally tagged union must be the direct child of a struct, and the tag identifier it specifies must be a previous-sibling member. Cases such as the following are not permitted, because they would make it possible to more easily spawn and serialize instances of the union orphaned from its tag (if not in C, then in C++).
    
    ```c++
    struct Container {
       int tag;
       struct {
          union {
             // ...
          } data;
       } member;
    };
    
    decltype(Container::member) orphaned;
    ```
    
    The following example is allowed to be made an externally-tagged union (and it's valid because the attribute would apply exclusively to the `data` field, not to its union type):
    
    ```c
    struct Container {
       int tag;
       union {
          // ...
       } data;
    };
    ```

The available attributes are:

<dl>
   <dt><code>__attribute__((lu_bitpack_union_external_tag("<var>name</var>")))</code></dt>
      <dd>
         <p>This attribute indicates that a union is externally tagged, and may be applied to any union-type member of a struct. The argument indicates the name of another member of that struct (or any member thereof) which will serve as a tag for this union. The named member must be of integral type, and must appear in the struct definition before the annotated union.</p>
         <p>This attribute is only valid when applied to a struct data member whose type is a union type.</p>
      </dd>
   <dt><code>__attribute__((lu_bitpack_union_internal_tag("<var>name</var>")))</code></dt>
      <dd>
         <p>This attribute indicates that a union is internally tagged, and may be applied to a union type or value. The argument indicates the name of a field that acts as the union's tag: the field must exist inside of all of the union's members (which must all be of struct types), at the same offset within each, with the same type within each; and that type must be an integral type.</p>
         <p>This attribute is only valid when applied to a union tag, a union typedef, or a struct data member whose type is a union type.</p>
      </dd>
   <dt><code>__attribute__((lu_bitpack_union_member_id(<var>n</var>)))</code></dt>
      <dd>
         <p>This attribute must be applied to each of a union's members, and they must all be given a unique integer constant <var>n</var>. If the union's tag value is equal to <var>n</var>, then its active member is the member whose tagged ID is <var>n</var>. If, at run-time (i.e. while your compiled program is reading or writing the union to a bitstream), the union's tag value isn't equal to the tagged IDs of any of its members, then the behavior is undefined.</p>
         <p>This attribute is only valid when applied directly to a member of a union.</p>
      </dd>
</dl>

Examples:

```c++
//
// Internally tagged union.
// &instance.a.tag == &instance.b.tag == &instance.c.tag.
//
union __attribute__((lu_bitpack_union_internal_tag("tag"))) InternallyTagged {
   __attribute__((lu_bitpack_tagged_id(0))) struct {
      int   header;
      int   tag;
      float weather;
   } a;
   __attribute__((lu_bitpack_tagged_id(1))) struct {
      int header;
      int tag;
      int climate;
      int humidity;
   } b;
   __attribute__((lu_bitpack_tagged_id(2))) struct {
      int header;
      int tag;
   } c;
};

//
// Externally tagged union. The union exists inside of a struct, 
// and a previously-seen member of that struct is the tag.
//
struct Foo {
   int tag;
   __attribute__((lu_bitpack_external_tag("tag"))) union {
      __attribute__((lu_bitpack_tagged_id(0))) struct {
         int   header;
         int   tag;
         float weather;
      } a;
      __attribute__((lu_bitpack_tagged_id(1))) struct {
         int header;
         int tag;
         int climate;
         int humidity;
      } b;
      __attribute__((lu_bitpack_tagged_id(2))) struct {
         int header;
         int tag;
      } c;
   } data;
};
```

### Generated XML

When running the plug-in, you can specify the path to an XML file as `-fplugin-arg-lu_bitpack-xml-out=$(DESIRED_PATH)/test.xml`. If you do so, then every time the plug-in generates serialization code, it will write information about the computed bitpacking format to the specified XML file. (Note that the `.xml` file extension is required and case-sensitive.)

The XML data has a single root `data` node which may contain the following children: `heritables`, `types`, `variables`, and `sectors`.

#### Types

A `struct-type` element denotes any non-anonymous `struct`-keyword type seen during code generation, and will have the following attributes:

<dl>
   <dt><code>name</code></dt>
      <dd>The unqualified name of the type.</dd>
   <dt><code>c-type</code></dt>
      <dd>The name of the type as produced by <code>gcc_wrappers::type::base::pretty_print</code>. This could potentially include qualifiers such as <code>volatile</code>.</dd>
</dl>

A `struct-type` element's children represent an ordered list of the struct's members. If the struct has an anonymous struct member, then the anonymous struct won't be listed; rather, its children are transitively included.

All member elements possess a `name` attribute indicating the field name, and a `c-type` attribute indicating the field's C language type.

Members whose bitpacking options indicate pre-pack or post-unpack transform functions may have `pre-pack-transform` and `post-unpack-transform` attributes noting the identifiers of those functions.

Additionally, if a member is an array, it will possess at least one `array-rank` child element, whose `extent` attribute indicates the array extent; for multi-dimensional arrays, these child elements are ordered matching left-to-right ordering of the array extents. (If a field is marked to be serialized as a string, then the deepest-nested array rank is not considered "an array." For example, if `char names[5][10]` is marked as a string, then this is considered a one-dimensional array, extent five, of ten-character strings; not a two-dimensional array.)

Members will have either a `default-value` attribute, or a `default-value-string` child element (whose text content is string content), if the elements were annotated with `lu_bitpack_default_value`. The default value is recorded even if it isn't used in the generated code (i.e. even if no such members are omitted from bitpacking).

The node name of a member element varies depending on its type, and some attributes are also type-specific:

* `omitted` elements represent fields that aren't serialized at all. These elements are only emitted if the fields have some other property worth capturing (e.g. their default value).
* `boolean` elements represent fields that are serialized as 1-bit booleans.
* `integer` elements represent fields that are serialized as integer values.
  * The `bitcount` attribute indicates the final used bitcount.
  * The `min` attribute indices the final used minimum value, such that in a bitstream, the serialized value will be *v - min*.
* `opaque-buffer` elements represent fields that are serialized as opaque buffers.
  * The `bytecount` attribute indicates the buffer size in bytes.
* `pointer` elements represent pointer fields.
  * The `bitcount` attribute indicates the final used bitcount.
* `string` elements represent string fields.
  * The `length` attribute indicates the final serialized length in characters.
  * The `with-terminator` attribute indicates whether the string requires a null terminator in memory. The null terminator is only serialized into the bitstream if it comes early (i.e. if a string's max length is 5, then "Lucia" does not serialize an 0x00 byte).
* `struct` elements represent nested structs.
* `union` elements represent tagged unions. (Support for these is not implemented yet.)
  * The `tag-type` attribute is either `external` or `internal`.
