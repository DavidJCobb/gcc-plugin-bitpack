
# gcc-plugin-bitpack

An attempt at building a GCC plug-in that can generate code for bitpacking data structures in a C program. Initial development guided heavily by [rofirrim/gcc-plugins](https://github.com/rofirrim/gcc-plugins); makefiles borrowed from there as well. (GCC plug-ins, this one included, are compulsorily GPL'd or GPL-compatible, so it's fine.)

As of this writing, I'm currently targeting GCC 11.4.0.

## Feature goals

* Annotate struct members with information specifying how they should be bitpacked
  * Set a bitcount explicitly or compute one dynamically.
  * Booleans should encode as single-bit by default.
  * Integral fields should be able to compute a bitcount based on their min and max values. Where these are not set, they default to the minimum and maximum representable values in the integral's type.
  * For strings, indicate whether they're null-terminated in memory.
  * Ability to categorize struct members, and associate bitpacking options with these categories (i.e. predefining sets of bitpacking options).
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

### Generated XML

When running the plug-in, you can specify the path to an XML file as `-fplugin-arg-lu_bitpack-xml-out=$(DESIRED_PATH)/test.xml`. If you do so, then every time the plug-in generates serialization code, it will write information about the computed bitpacking format to the specified XML file. (Note that the `.xml` file extension is required and case-sensitive.)

The XML data has a single root `data` node which may contain the following children: `heritables`, `types`, `variables`, and `sectors`.

#### Heritables

All sets of heritable options are output as `heritable` elements within the `data>heritables` element. The following attributes are present on all such elements:

<dl>
   <dt><code>name</code></dt>
      <dd>The name of this set of heritable options. Must be unique.</dd>
   <dt><code>type</code></dt>
      <dd>The type of this set of heritable options: <code>integer</code> or <code>string</code>.</dd>
</dl>

The following attributes may also be present on all such elements:

<dl>
   <dt><code>pre-pack-transform</code></dt>
      <dd>The identifier of a function that should be used to transform the value before it is packed and serialized.</dd>
   <dt><code>post-unpack-transform</code></dt>
      <dd>The identifier of a function that should be used to transform the value after it is unpacked and before it is stored.</dd>
</dl>

For integer-type options, the following attributes may be present:

<dl>
   <dt><code>bitcount</code></dt>
      <dd>An integer constant representing the requested bitcount for fields inheriting these options.</dd>
   <dt><code>min</code></dt>
      <dd>An integer constant representing the minimum value for fields inheriting these options.</dd>
   <dt><code>max</code></dt>
      <dd>An integer constant representing the maximum value for fields inheriting these options.</dd>
</dl>

For string-type options, the following attributes may be present:

<dl>
   <dt><code>length</code></dt>
      <dd>An integer constant representing the expected length of string fields inheriting these options.</dd>
   <dt><code>with-terminator</code></dt>
      <dd>The value <code>true</code> or <code>false</code>, indicating whether a string field must have a null terminator. (If so, the null terminator is not counted in the <code>length</code>.)</dd>
</dl>

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

Members that inherit any set of heritable options will have an `inherit` attribute indicating the name of the heritable option set. Members whose bitpacking options indicate pre-pack or post-unpack transform functions may have `pre-pack-transform` and `post-unpack-transform` attributes noting the identifiers of those functions.

Additionally, if a member is an array, it will possess at least one `array-rank` child element, whose `extent` attribute indicates the array extent; for multi-dimensional arrays, these child elements are ordered matching left-to-right ordering of the array extents. (If a field is marked to be serialized as a string, then the deepest-nested array rank is not considered "an array." For example, if `char names[5][10]` is marked as a string, then this is considered a one-dimensional array, extent five, of ten-character strings; not a two-dimensional array.)

The node name of a member element varies depending on its type, and some attributes are also type-specific:

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
