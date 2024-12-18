
# gcc-plugin-bitpack

An attempt at building a GCC plug-in that can generate code for bitpacking data structures in a C program. Initial development guided heavily by [rofirrim/gcc-plugins](https://github.com/rofirrim/gcc-plugins); makefiles borrowed from there as well. (GCC plug-ins, this one included, are compulsorily GPL'd or GPL-compatible, so it's fine.)

As of this writing, I'm currently targeting GCC 11.4.0.

## Features

* Annotate struct members with information specifying how they should be bitpacked
  * Integral fields can be annotated with their minimum and maximum possible values, from which we'll compute the minimum bitcount necessary to pack them; or you can choose a bitcount explicitly.
  * Booleans encode as a single bit by default.
  * Values can be marked as strings to use alternate bitpacking functions, and can additionally be marked as not requiring a null terminator when residing unpacked in memory.
  * Tagged unions can be annotated to facilitate bitpacking of their contents.
* Automatically generate code to serialize structs to a bitpacked format, and read them back from that format.
  * Optionally divide bitpacked data into "sectors" of a limited size. Serialization should pause at the end of one sector &mdash; even if this means serializing only part of a struct or array &mdash; and resume where it left off when beginning the next sector. Sectors can be serialized in any order.
    * Sector size
    * Sector count
* Export an XML file describing the resulting bitpacked format.
  * This is meant to aid with structs that may change, by facilitating the development of external tools that (guided by the XML) can read data in an older format and convert it to a newer one.
  * This can also offer statistics useful for retrofitting this bitpacking scheme into older programs: the report can contain statistics that can help you measure which pieces of data are more "expensive" or appear more frequently. This can help you reason about how much space you have to work with and how much space certain changes may consume.
* Supported versions
  * I aim to support the lowest and highest minor version for every major version after GCC 11.4.0.
    * This includes 11.5.0, the highest minor version for GCC 11.
    * As of this writing, I've run basic builds and tests up to GCC 14.2.0.
  * The plug-in has been most extensively tested with GCC 11.4.0, since that's just what I ended up with when I started using WSL.

## Non-goals

* Presence bits (think "`std::optional`") and variable-length data. My current use case entails packing data into a limited storage space, such that the largest possible data must fit. Ergo bitpacked data must always be uniform in size, so we can check statically whether we have enough room.
  * Tagged unions are zero-padded as necessary, so that all "branches" of the union have equal-size serialized representations.
* Support for use in precompiled headers. This would require altering the plug-in to be `gengtype`-friendly, which is not currently in scope.

## Advanced features

### Transformations

It is sometimes useful to be able to apply automatic transformations to a value when it's serialized to a bitstream and read from a bitstream. This plug-in allows you to define transformations between different value types.

For example, imagine a video game inventory that is subdivided into different categories &mdash; not just in terms of how we present data to the user, but in terms of the underlying in-memory representation as well &mdash; in an item system where every item has a globally unique ID. For example, you may have a "weapons" category and an "armor" category, with items like these:

| Global ID | Item |
| -: | :- |
| 0 | None |
| 1 | Wooden Sword |
| 2 | Stone Sword |
| 3 | Iron Sword |
| 4 | Gold Sword |
| 5 | Diamond Sword |
| 6 | Iron Armor |
| 7 | Gold Armor |
| 8 | Diamond Armor |

The highest possible global item ID is 8, requiring four bits to encode. If you can carry up to 10 weapons and up to 5 armors, then this means we need 4 &times; 15 = 60 bits for the entire inventory. Recall, however, that the inventory is categorized under the hood as well. Why not take advantage of that for serialization?

It will generally be easiest to use global item IDs in memory, while the game is running; but for the sake of serialization, we can give each item category its own "local" ID space:

| Global ID | Local ID | Item |
| -: | -: | :- |
| 0 | 0 | None |
| 1 | 1 | Wooden Sword |
| 2 | 2 | Stone Sword |
| 3 | 3 | Iron Sword |
| 4 | 4 | Gold Sword |
| 5 | 5 | Diamond Sword |
| 6 | 1 | Iron Armor |
| 7 | 2 | Gold Armor |
| 8 | 3 | Diamond Armor |

The highest local weapon ID is 5, requiring only 3 bits to encode; and the highest local armor ID is 3, requiring only 2 bits to encode; and so now, the total serialized size is (3 * 10) + (2 * 5) = 40 bits. The thing is, this requires being able to transform the in-memory data before it's bitpacked, and transform the bitpacked data after it's read but before it's stored.

Taking advantage of this idea, we can do this:

```c
#define LU_BP_MAX(n) __attribute__((lu_bitpack_range(0, n)))
#define LU_BP_TRANSFORM(pre, post) __attribute__((lu_bitpack_funcs("pre_pack=" #pre ",post_unpack=" #post )))

struct PackedInventory {
   LU_BP_MAX(5) uint8_t weapons[10];
   LU_BP_MAX(3) uint8_t armors[5];
};

void MapInventoryForSave(const struct Inventory* in_memory, struct PackedInventory* packed);
void MapInventoryForRead(struct Inventory* in_memory, const struct PackedInventory* packed);

LU_BP_TRANSFORM(MapInventoryForSave, MapInventoryForRead)
struct Inventory {
   uint8_t weapons[10];
   uint8_t armors[5];
};
```

(Yes, the example above could be accomplished using the `lu_bitpack_range(min, max)` attribute, but only because I made the item categories contiguous for clarity. In a real-world scenario, global item IDs may not be sorted by category: weapons and armor may be interleaved together, and in that case, you'd need a more complex mapping between global and category-local IDs.)

There are some constraints that the transform functions have to operate under; refer to the documentation for transform options below.


## Building

The makefiles assume that you've downloaded and installed GCC in `$HOME/gcc/build/11.4.0` or the equivalent folder for other GCC versions, and that `$HOME/gcc/build/11.4.0/gmp` contains built GMP sources.

If you `sudo apt install lua5.4`, you can then use `lua setup.lua --version 11.4.0 --fast` to install any given GCC version and ensure that GMP is set up appropriately. (Versions of GCC prior to 13.1.0 only place GMP in the GCC objdir if you do a slow bootstrapping build; the script detects GMP's absence in that scenario and builds it to that spot for you, so the plug-in makefiles can reference it.) Alternatively, there is `setup.sh` which does the same thing, but which I don't intend to maintain anymore (bash is as painful as its name would imply).

From within the plug-in source directory: running testcases:

```
make GCCVER=13.1.0 testname=codegen-various-a testcase
```

Substitute `13.1.0` for the version of GCC to build for, and `codegen-various-a` for a folder name in the `testcases` subfolder. Bear in mind that GCC 13+ requires newer versions of Ubuntu than 22.04, which is the default for WSL as of this writing.


## Usage

### Pragmas

#### `enable`

This pragma activates the plug-in's functionality. You should use it in the translation unit where you intend to generate functions.

```c
#pragma lu_bitpack enable
```

Most of the plug-in's logic, including most attribute validation, will not run unless you've `enable`d the plug-in's functionality. This is intended to avoid slowing down builds when bitpacking attributes are used within frequently-referenced headers.

Code generation will fail if `enable` isn't used before `generate_functions`, or if any bitpacking attributes are seen before `enable` is used.

#### `set_options`

This pragma sets global bitpacking options. It must be run after the `enable` pragma and before the `generate_functions` pragma.

Most of these options are used to tell the generated code where to find bitpacking-related functions. The general design is to have a "bitstream state" struct that wraps a buffer pointer and an offset, and is passed to each individual serialization function to read or save a value (with the same struct used for reading and writing to the buffer).

| Option name | Required | Type | Description |
| :- | :-: | :- | :- |
| `sector_count` | Optional | integer | Maximum number of sectors to generate code for. If the to-be-serialized values end up being split into more than this many sectors, code generation will fail. |
| `sector_size` | Optional | integer | Maximum size in bytes of each sector. If not specified, then there will be no limit. |
| `bitstream_state_typename` | Required | typename | Name of a bitstream state struct type. |
| `bool_typename` | Optional | typename | Name of an integral type that should be treated as a boolean type; if not specified, defaults to `bool`. Exists to help with older C dialects that don't define `bool` as its own type. |
| `buffer_byte_typename` | Required | typename | Name of a single-byte integral type. |
| `func_initialize` | Required | function identifier | Identifier of a function with signature `void f(const bitstream_state_typename*, buffer_byte_typename*)` used to initialize the bitstream state. |
| `func_read_bool` | Required | function identifier | Identifier of a function with signature `bool_typename f(bitstream_state_typename*)` used to read a boolean serialized as a single bit. |
| `func_read_u8` | Required | function identifier | Identifier of a function with signature `uint8_t f(bitstream_state_typename*, uint8_t bitcount)` used to read a `uint8_t` with a given serialized bitcount. |
| `func_read_u16` | Required | function identifier | Identifier of a function with signature `uint16_t f(bitstream_state_typename*, uint8_t bitcount)` used to read a `uint16_t` with a given serialized bitcount. |
| `func_read_u32` | Required | function identifier | Identifier of a function with signature `uint32_t f(bitstream_state_typename*, uint8_t bitcount)` used to read a `uint32_t` with a given serialized bitcount. |
| `func_read_s8` | Required | function identifier | Identifier of a function with signature `int8_t f(bitstream_state_typename*, uint8_t bitcount)` used to read a `int8_t` with a given serialized bitcount. |
| `func_read_s16` | Required | function identifier | Identifier of a function with signature `int16_t f(bitstream_state_typename*, uint8_t bitcount)` used to read a `int16_t` with a given serialized bitcount. |
| `func_read_s32` | Required | function identifier | Identifier of a function with signature `int32_t f(bitstream_state_typename*, uint8_t bitcount)` used to read a `int32_t` with a given serialized bitcount. |
| `func_read_buffer` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, buffer_byte_type* dst, uint16_t max_length)` used to read a serialized opaque buffer. |
| `func_read_string_nt` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, char* string, uint16_t max_length)` used to read a serialized string that requires a null terminator in memory. |
| `func_read_string` | Required | function identifier | Synonym for `func_read_string_nt`. You only need to specify one of them. |
| `func_read_string_ut` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, char* string, uint16_t max_length)` used to read a serialized string that doesn't require a null terminator in memory. |
| `func_save_bool` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, bool_typename)` used to save a boolean, serialized it as a single bit. |
| `func_save_u8` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, uint8_t value, uint8_t bitcount)` used to save a `uint8_t` with a given serialized bitcount. |
| `func_save_u16` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, uint16_t value, uint8_t bitcount)` used to save a `uint16_t` with a given serialized bitcount. |
| `func_save_u32` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, uint32_t value, uint8_t bitcount)` used to save a `uint32_t` with a given serialized bitcount. |
| `func_save_s8` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, int8_t value, uint8_t bitcount)` used to save a `int8_t` with a given serialized bitcount. |
| `func_save_s16` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, int16_t value, uint8_t bitcount)` used to save a `int16_t` with a given serialized bitcount. |
| `func_save_s32` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, int32_t value, uint8_t bitcount)` used to save a `int32_t` with a given serialized bitcount. |
| `func_save_buffer` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, const buffer_byte_type* dst, uint16_t max_length)` used to save an opaque buffer. |
| `func_save_string_nt` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, const char* string, uint16_t max_length)` used to save a serialized string that requires a null terminator in memory. |
| `func_save_string` | Required | function identifier | Synonym for `func_save_string_nt`. You only need to specify one of them. |
| `func_save_string_ut` | Required | function identifier | Identifier of a function with signature `void f(bitstream_state_typename*, const char* string, uint16_t max_length)` used to save a serialized string that doesn't require a null terminator in memory. |

#### `generate_functions`

This pragma triggers generation of bitpack-related functions and saving of any enabled XML output. This pragma must be used at file scope, and you must have already `enable`d code generation and `set_options` via the respective pragmas.

```c
#pragma lu_bitpack generate_functions( \
   read_name = generated_read,         \
   save_name = generated_save,         \
   data      = sTestStruct,            \
   enable_debug_output = true \
)
```

Options are key/value pairs, with pairs being comma-separated and keys being separated from their values with a `=` glyph. The options are as follows:

<dl>
   <dt><code>read_name</code></dt>
      <dd>
         <p>Identifier of the function to generate for reading a bitstream. This function must have the signature <code>void func_name(const buffer_byte_type* src, int sector_id)</code> given the buffer byte type specified in the global options. The function will be implicitly declared if you haven't already declared it; and the function must not have already been defined.</p>
      </dd>
   <dt><code>save_name</code></dt>
      <dd>
         <p>Identifier of the function to generate for reading a bitstream. This function must have the signature <code>void func_name(buffer_byte_type* dst, int sector_id)</code> given the buffer byte type specified in the global options. The function will be implicitly declared if you haven't already declared it; and the function must not have already been defined.</p>
      </dd>
   <dt><code>data</code></dt>
      <dd>
         <p>A list of identifiers (of top-level variables) to read/save into the serialized bitstream. Identifiers must be separated with whitespace or with a <code>|</code> glyph; the latter case will push the right-side identifier into the next sector.</p>
      </dd>
   <dt><code>enable_debug_output</code></dt>
      <dd>
         <p>This must be either an integer literal, or the identifiers <code>true</code> or <code>false</code>. If it is non-zero or <code>true</code>, then we will log debug output during code generation.</p>
      </dd>
</dl>

If successful, code generation will define (and implicitly declare, if needed) the requested read and save functions. Additionally, the following symbols will be defined:

<dl>
   <dt><code>__lu_bitpack_sector_count</code></dt>
      <dd>
         <p>A <code>size_t</code>-type variable declared and defined within the current translation unit, equaling the number of sectors generated. If the current environment supports it, the variable will be declared <code>constexpr</code> as well.</p>
      </dd>
   <dt><code>__lu_bitpack_max_sector_size</code></dt>
      <dd>
         <p>A <code>size_t</code>-type variable declared and defined within the current translation unit. If the current environment supports it, the variable will be declared <code>constexpr</code> as well.</p>
         <p>If your global options cap the sector count at 1 <em>and</em> set no max limit on the sector size, then the value of this variable will be the bytecount (rounded up to the nearest byte) of the sole generated sector. Otherwise, it will be the max sector size in bytes. This means that for the non-sectored use case &mdash; "just dump everything into a big ol' buffer" &mdash; you can use this variable to know how big that ol' buffer ended up being, and know how much memory to allocate for stream reads and writes.</p>
      </dd>
   <dt><code>__lu_bitpack_read_sector_<var>n</var></code> for integer <var>n</var></dt>
   <dt><code>__lu_bitpack_save_sector_<var>n</var></code> for integer <var>n</var></dt>
      <dd>
         <p>Per-sector functions generated and called by the top-level read and save functions. These are exposed to user code so that they can be targeted with this plug-in's debugging pragmas; future versions of the plug-in may cloak these functions and offer dedicated pragmas for dumping information about them.</p>
      </dd>
   <dt><code>__lu_bitpack_read_sector_<var>T</var></code> for typename <var>T</var></dt>
   <dt><code>__lu_bitpack_save_sector_<var>T</var></code> for typename <var>T</var></dt>
      <dd>
         <p>Functions generated whenever an entire struct or union <code><var>T</var></code> is generated via a single function call. These are exposed to user code so that they can be targeted with this plug-in's debugging pragmas; future versions of the plug-in may cloak these functions and offer dedicated pragmas for dumping information about them.</p>
      </dd>
</dl>

#### `debug_dump_bp_data_options`

Dumps the computed bitpacking options of a given identifier to the console. You can specify nested identifiers using `::`, and as of this writing, you can refer to types or declarations.

```c++
static struct TestStruct {
   int foo;
   int bar;
} sTestStruct;

#pragma lu_bitpack debug_dump_bp_data_options TestStruct::foo
#pragma lu_bitpack debug_dump_bp_data_options sTestStruct::bar
```

#### `debug_dump_function`

Dumps information about a given function (specified by its identifier) to the console.

```c++
#pragma lu_bitpack debug_dump_function main
```

This was used during plug-in development. It may be removed in future versions.

#### `debug_dump_identifier`

Dumps information about a given identifier to the console.

```c++
#pragma lu_bitpack debug_dump_identifier main
```

This was used during plug-in development. It may be removed in future versions.

### Attributes

Attributes may be applied to non-array types or to struct field declarations in order to configure how those fields are serialized. Some attributes can also be used on structure and union tags[^attributes-on-tags].

[^attributes-on-tags]:

    There are two ways of defining a named `struct` or `union` type: by using the `typedef` keyword on an otherwise anonymous type, or by defining a <dfn>tag</dfn>:
    
    ```c
    struct TagName {
       // ...
    };
    ```
    
    When using a tag, you must place attributes after the `struct` keyword and before the tag name. Attributes placed ahead of the keyword will silently fail in GCC.
    
    ```c
    __attribute__((discarded_attr)) struct TagName {
    };
    
    struct __attribute__((retained_attr)) struct TagName {
    };
    ```

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

The following attributes may be used on variable and field declarations only:

<dl>
   <dt><code>lu_bitpack_stat_category("<var>name</var>")</code></dt>
      <dd>
         <p>Applies a given name to the declaration for stat-tracking purposes. The XML output, if enabled, will report how many values of a given category appear in various parts of the serialized output. In the case of arrays, the element type is used unless the array is marked as a string (see below), in which case the innermost array type is considered "one instance."</p>
         <p>This attribute can be used multiple times on the same declaration, with different names, and the values will be considered members of each named category.</p>
      </dd>
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

Additional requirements exist for pre-pack and post-unpack functions:

* These functions must be symmetric.
* It must be valid to invoke these functions on partially-constructed values.
  * These functions must not crash or fail on incomplete or invalid data.
  * These functions must not assume that data which appears to be valid actually is valid.
  * These functions must not have side-effects that depend on data being valid. (Save those for after you finish reading the entire bitstream.)
  * These functions must not assume that because one piece of data appears to be valid, any other piece of data is also valid.

These requirements exist because a transformed value may be split across multiple sectors, and special measures have to be taken to prevent fields read in a later sector from clobbering fields read in an earlier sector.

The naive approach to reading a transformed object would be as follows:

* Construct an uninitialized transformed object.
* Read the object's data.
* Call the post-unpack function to convert the transformed object back to its in situ type.

However, when the transformed object is split across multiple sectors, this process ends up becoming:

* Construct an uninitialized transformed object.
* Read the first few fields.
* Call the post-unpack function to convert the (incomplete) transformed object back to its in situ type.
* Move to the next sector.
* Construct an uninitialized transformed object.
* Read the last few fields.
* Call the post-unpack function to convert the (incomplete) transformed object back to its in situ type. The fields read in the previous sector will be clobbered, because they were not present in this sector's transformed object.

To prevent this, we use the following process whenever it appears as though a transformed object has been divided across sectors:

* Construct an uninitialized transformed object.
* Read the first few fields.
* Call the post-unpack function to convert the (incomplete) transformed object, producing an (incomplete) in situ object.
* Move to the next sector.
* Construct an uninitialized transformed object.
* Call the pre-pack function to convert the (incomplete) in situ object, overwriting the transformed object. Because the pre-pack and post-unpack functions are supposed to be symmetric, this means that the fields we read in the previous sector will be converted and written into the current sector's transformed object, preventing them from being lost.
* Read the last few fields.
* Call the post-unpack function to convert the now-complete transformed object back to its in situ type.

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
         <p>This attribute indicates that a union is internally tagged, and may be applied to a union type or value. The argument indicates the name of a field that acts as the union's tag. All of the union's members must be of struct types; those structs' first <var>n</var> members must be identical for some value of <var>n</var> &gt; 0; and the tag must be one of those members, and its type must be an integral type.</p>
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

When running the plug-in, you can specify the path to an output XML file. This file will contain a representation of the serialization format that this plug-in generates code for, as well as information that can be processed to measure stats about the packed output (e.g. space-efficiency, etc.).

See [README - XML OUTPUT](README%20-%20XML%20OUTPUT.md) for further details.

### Notes

* Typedefs are not treated as strictly equivalent to the original type, because you can attach bitpacking options to the `typedef` itself. The plug-in makes no effort to consider typedefs equivalent even in cases where no options are applied to them. Given `struct A` and `typedef struct A B`, if the to-be-serialized output contains values that use both typename `A` and typename `B`, these will be treated as separate types. The XML output will have separate `<struct>` elements for each typename, and if at least one instance of each typename fits wholly within a sector, code generation will produce separate functions for each, even if their bitpacking options would be identical and thus the functions would be identical.
