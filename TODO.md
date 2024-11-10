
# To-do

## `lu-bitpack`

### Short-term

* To fully implement transform functions for structs (both generating code to call them, and including them in the attributes for `struct-type` XML elements), we need to modify `codegen::struct_descriptor` to store any [computed] bitpacking options that are appropriate for struct types.
* See "Rethink heritables" section below.
* The handler for `lu_bitpack_inherit` should verify that the specified heritable is of a type compatible with the annotated type/field.
* We should add a plug-in callback for when a `DECL` is finished. There,...
  * If the `FIELD_DECL` inherits a `string` heritable, we should check the field's string length and whether it has the `nonstring` attribute. If either of these don't match the options specified on the heritable, we should error.
* I don't think our `std::hash` specialization for wrapped tree nodes is reliable. Is there anything else we can use?
  * Every `DECL` has a unique `DECL_UID(node)`, but I don't know that these would be unique among other node types' potential ID schemes as well.
  * `TYPE_UID(node)` also exists.
  * Our use case is storing `gw::type::base`s inside of an `unordered_map`. We could use GCC's dedicated "type map" class, or &mdash; and this may be more reliable &mdash; we could use a `vector` of `pair`s, with a lookup function which looks for an exact type match or a match with a transitive typedef.

* It's possible to set bitpacking options on an array `typedef` but due to how we handle coalescing of data options, these options would never be seen (unless, perhaps, they were set on an array of the string character type specified in the global bitpacking options). Do we want to fix this?
* We should get rid of the ability to specify a character type for bitpacking options, and instead just allow string bitpacking options to be applied to any array, provided that that array is of `char`, `signed char`, `unsigned char`, or any variant type (and currently, `uint8_t` and typedefs thereof count as variants of `unsigned char`).
  * This may require redoing a lot of the logic that depends on the "innermost value type" of a field as understood by the global bitpacking options.

* The XML output has no way of knowing or reporting what bitpacking options are applied to integral types (as opposed to fields *of* those types). The final used bitcounts (and other associated options) should still be emitted per field, but this still reflects a potential loss of information. Can we fix this?
* It'd be very nice if we could find a way to split buffers and strings across sector boundaries, for optimal space usage.
  * Buffers are basically just `void*` blobs that we `memcpy`. When `sector_functions_generator::_serialize_value_to_sector` reaches the "Handle indivisible values" case (preferably just above the code comment), we can check if the primitive to be serialized is a buffer and if so, manually split it via similar logic to arrays (start and length).
  * Strings require a little bit more work because we have to account for the null terminator and zero-filling: a string like "FOO" in a seven-character buffer should always load as "FOO\0\0\0\0"; we should never leave the tail bytes uninitialized. In order to split a string, we'd have to read the string as fragments (same logic as splitting an array) and then call a fix-up function after reading the string (where the fix-up function finds the first '\0', if there is one, and zero-fills the rest of the buffer; and if the string requires a null-terminator, then the fix-up function would write that as well). So basically, we'd need to have bitstream "string cap" functions for always-terminated versus optionally-terminated strings.
* Decisions to be made
  * GCC has a `__attribute__((nonstring))` attribute for strings that may not always be null-terminated. Instead of having `lu_bitpack_string` indicate the requirement for a null terminator (and worse: defaulting that to "no"), it might be better to require that potentially unterminated strings be annotated with `nonstring`, and to have the bitpacking codegen and XML react to `nonstring`.
    * `nonstring` was added to GCC in 2017 circa GCC 8.0.
* Long-term plans below
  * Pre-pack and post-unpack functions
  * Union support

### Short-term advanced

#### Re-think heritables

Heritables were introduced in the old XML-based proofs of concept for two reasons:

* Reusing sets of options to ensure consistency between fields located in different structs.
* Statistics: the old proof of concept was able to produce a comprehensive report indicating how much space was used by different structs and heritables, and how much space was saved by bitpacking them.

With the plug-in, consistency is best attained through typedefs with options set:

```c
#define LU_BP_BITCOUNT(n) __attribute__((lu_bitpack_bitcount(n)))
#define LU_BP_MAX_ONLY(n) __attribute__((lu_bitpack_range(0,n)))
#define LU_BP_RANGE(x,y)  __attribute__((lu_bitpack_range(x,y)))

enum {
   LANG_UNDEFINED,
   LANG_ENGLISH,
   LANG_FRENCH,
   LANG_GERMAN,
   LANG_ITALIAN,
   LANG_JAPANESE,
   LANG_KOREAN,
   LANG_SPANISH,
   
   LANG_COUNT
};

LU_BP_BITCOUNT(LANG_COUNT) typedef uint8_t language_id;
```

Theoretically we could have an attribute that enables stat-tracking &mdash; keeping track of how many times a given type occurs in the serialized output:

```c
// Count how many times this type appears in the output, and how many times it appears 
// in each seen struct type.
#define LU_BP_TRACK __attribute__((lu_bitpack_track_statistics))

LU_BP_TRACK LU_BP_BITCOUNT(LANG_COUNT) typedef uint8_t language_t;
```

Something like that would work for anything other than strings because when we process bitpacking options, we follow typedef chains to do so rather than just shortcutting to the main/canonical types. That is: given `typedef original newname`, options set on `original` are inherited (and can be overridden) by `newname`, but options set on `newname` don't propagate backward to `original`.

The problem is that this falls short for strings. We can set bitpacking options on array types, but nothing actually reads them, because we only care about the innermost value type. (We make a specific exception for things tagged as strings, using the innermost *array* type.)

```c
// This doesn't work: the options would never be seen.
LU_BP_TRACK typedef char player_name_t [7];

// Syntax reminder: `typedef a b [n]` defines an alias to `a[n]`.
// (It's meant to match how field declarations work, similar to 
// the whole `int *a, b, c` debacle.)
```

There's an additional limitation as well, which is that GCC's `__attribute__((nonstring))` doesn't work on types or typedefs; only fields.

The goal we should work toward, then, is:

* Figure out a more coherent way to deal with attributes on arrays &mdash; something that will work for strings.
* Implement an attribute that marks a type for statistics-tracking.
* Implement generating a statistics report for serialization, *or* augment the XML output with enough information for outside tools to generate such a report.
* Get rid of heritables in favor of statistics-tracked typedefs.

##### Thoughts

The following should "just work:"

```c
#define LU_BP_BITCOUNT(n) __attribute__((lu_bitpack_bitcount(n)))

#define LU_BP_STRING    __attribute__((lu_bitpack_string))
#define LU_BP_STRING_NT __attribute__((lu_bitpack_string))
#define LU_BP_STRING_UT __attribute__((lu_nonstring)) LU_BP_STRING

struct Foo {
   LU_BP_STRING_UT char player_name[7];
   
   LU_BP_STRING_UT char player_name[10][7]; // ten player names
};

LU_BP_STRING_UT typedef char player_name_t [7];

struct Bar {
   player_name_t player_name;
   
   player_name_t player_name[10];
};

LU_BP_BITCOUNT(3) typedef uint8_t small_number;
struct Baz {
   LU_BP_BITCOUNT(3) uint8_t smalls_a[7]; // total bitcount: 3 * 7 == 21
   
   small_number smalls_b[7]; // total bitcount: 3 * 7 == 21
};
```

So let's come up with some rules.

* If the `lu_bitpack_string` attribute is applied to a `TYPE_DECL`, then the declared type must be an array of single-byte values.
* If the `lu_bitpack_string` attribute is applied to a `FIELD_DECL` or `VAR_DECL`, then the declaration must be an array, and the innermost array type must meet the rules for applying the attribute to `TYPE_DECL`s.
* If a `FIELD_DECL` has the attribute, or if the innermost array type has the attribute, then its innermost array rank is treated as a string.

```c
struct Foo {
   /*
      for(int i = 0; i < 4; ++i)
         lu_BitstreamRead_String(state, names[i], 7 - 1);
   */
   LU_BP_STRING names[4][7];
};
```

What about buffers?

* If the `lu_bitpack_as_opaque_buffer` attribute is applied to a `TYPE_DECL`, then the declared type is treated as an opaque buffer.
* If the `lu_bitpack_as_opaque_buffer` attribute is applied to a `FIELD_DECL` or `VAR_DECL`, then the declaration is treated as an opaque buffer.
* If a `FIELD_DECL` has the attribute, or if its type has the attribute, or if its type is [an array of [arrays of [...]]] a type that has the attribute, then the entire `FIELD_DECL` should be treated as a single opaque buffer and serialized with a single function call. Statistics-tracking must still count the field appropriately (i.e. `T field[x][y]` counts as <var>x</var> &times; <var>y</var> occurrences of <code>T</code>).

```c
LU_BP_TRACK LU_BP_AS_OPAQUE_BUFFER typedef uint8_t buffer [64];

struct Foo {
   // Serializes with a single memcpy call, size 64.
   // Statistics logs it as one instance.
   buffer a;
   
   // Serializes with a single memcpy call, size 128.
   // Statistics logs it as two instances.
   buffer a[2];
};
```

And for other attributes? Let's use `lu_bitpack_bitcount` as a case study.

* If the `lu_bitpack_bitcount` attribute is applied to a `TYPE_DECL`, then the declared type must be either an integral type, or an array of [arrays of [...]] an integral type.
* If the `lu_bitpack_bitcount` attribute is applied to a `FIELD_DECL`, then its type must meet the rules for applying the attribute to `TYPE_DECL`s.
* If a `FIELD_DECL` has the attribute, or if its type has the attribute, or if its type is [an array of [arrays of [...]]] a type that has the attribute, then the outermost specified bitcount is used for all elements therein.

Let's explore this further.

```c
// std::is_same_v<smalls, uint8_t[7]> and the serialized size is 5 * 7 == 35
LU_BP_BITCOUNT(5) typedef uint8_t smalls [7];

// std::is_same_v<smallers, uint8_t[2][7]> and the serialized size is 4 * (7 * 2) == 56
LU_BP_BITCOUNT(4) typedef smalls smallers[2];

struct Foo {
   // the serialized size is 3 * (7 * 2) == 42
   LU_BP_BITCOUNT(3) smallers smallests;
};

// the requested sizes, ordered from outermost to innermost, are: 3, 4, 5
// given the FIELD_DECL and the array ranks
// and we use the outermost size
```


### Long-term

#### Pre-pack and post-unpack functions

**BLUF:** Bitpacking involves finding the most compact possible representation of a given struct. However, we may wish to transform certain structs just before they're saved or just after they're read, so that they can be compacted further. We should offer a way to specify functions that can be called automatically to perform these operations. We currently allow the user to specify the names of functions to transform a data member, but we don't allow them to specify this on a type definition (we simply don't notice it there), nor have we actually implemented code to invoke those functions.

My use case involves some kinds of data that ought to be packed in a completely separate format from the way it's stored in memory.

Imagine a video game inventory that is subdivided into different categories &mdash; not just in terms of how we present data to the user, but in terms of the underlying in-memory representation as well &mdash; in an item system where every item has a globally unique ID. For example, you may have a "weapons" category and an "armor" category, with items like these:

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

Taking advantage of this idea using a future version of this plug-in might look like this:

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

(Yes, the example above could be accomplished using the `lu_bitpack_range(min, max)` attribute, but only because I made the item categories contiguous for clarity. In a real-world scenario, global item IDs may not be sorted by category: weapons and armor may be interleaved together, and in that case, you'd need a more complex mapping between global and category-local IDs.

(Additionally, it's valuable to have the generated serialization code handle data transformations because, for example, `Inventory` may be a member of some far larger struct that we want to save, rather than a freestanding struct that we can manually convert before and after initiating the save and load operations.)

##### Details

We need to be able to change the type of the value we're packing. This means that we have to generate code like the following for save:

```c
// local block to limit the scope of `__v`
{
   // generated variable
   struct PackedInventory __v;
   
   // extra call
   MapInventoryForSave(&currentStruct->inventory, &__v);
   
   __lu_bitpack_write_PackedInventory(state, &__v);
}
```

And like the following for read:

```c
{
   // generated variable
   struct PackedInventory __v;
   
   __lu_bitpack_read_PackedInventory(state, &__v);
   
   // extra call
   MapInventoryForRead(&currentStruct->inventory, &__v);
}
```

This in turn means that we need to infer the type of `__v` from the types used in the bitpack functions. That is:

* Let the *in situ* type of the to-be-packed member be *T*.
* Let the *transformed* type of the to-be-packed member be *U*. This is the type that will be used for the generated variable `__v` above.
* The pre-pack function must be of the form `void a(const T*, U*)`.
* The post-unpack function must be of the form `void b(T*, const U*)`.

The actual logistics of this include:

* When we generate computed bitpacking options for a to-be-transformed member, we need to base those (and the `member_kind`) on the transformed type, not the in situ type. We will of course still need to be aware of the original type, and of the fact of there being a transformation.
  * But wait. We currently validate a lot of options when we see the attributes for them, such as erroring if we see integer options on a non-integral type or field. That... isn't going to play so well with transforms.
    * Maybe we should just flat-out make it so that transforms can't be used alongside other options: if you use a transform on a type or field, then you have to specify all further bitpacking options on the transformed type, not on the in situ type or field.

Other support requirements:

* We should rename `lu_bitpack_funcs` to `lu_bitpack_transform`.
* When using transformation functions, the `member_kind` of a to-be-serialized member should depend on the transformed type, not the in situ type.
  * ...*but* we should still enforce per-type limits based on the data member's original type. That is: a struct-type data member is forbidden from specifying integer-type bitpacking options *even if* it uses transformation functions to be serialized as an integer. Those options should instead be specified on the integer type (and if it's a built-in e.g. `uint8_t`, then use a typedef and put the options on that typedef; I've already implemented error messaging in the `lu_bitpack_range` attribute handler instructing users to do this).


#### Union support

**BLUF:** Currently, we don't support unions at all. We could add preliminary support for them by allowing the user to mark them as opaque buffers, but we want to support bitpacking them in the future. Bitpacking unions poses a challenge primarily due to the need to split unions across sector boundaries for the most robust packing possible.

The current approach to bitpacking is always-forward: we generate one per-sector function at a time and track the amount of bits currently remaining in that sector. In order to properly split unions, however, we'll want to instead preemptively "open" every per-sector function at the start of generation, and then emit code into the "current" function. When we encounter a union and we can tell that it won't fit in the current sector, we'll...

1. Create an if/else tree branching on the union's tag, in both the current sector and the next sector. We want one branch per serializable union member.
2. For each union member *n*...
   1. Generate as much code into the *n* branch in the current sector.
   2. Emit the rest of the code into the *n* branch in the next sector.
   3. If *n* is not the largest member in the union, then pad with zeroes until the bitpacked sizes match.
3. Once all union members are done, advance code generation as a whole to the next sector.

This approach can generalize to unions that need to split multiple times, because they are large enough (and positioned close enough to the end of one sector) to span across three or more sectors.

Of course, unions won't only contain primitive members; they may also contain nested structs, arrays, and even other unions. This means that we'd have to modify the entire overall algorithm, conceptualizing ourselves as always emitting code into one branch among potentially several, and tracking bitcounts for the current branch; and that branch must itself be able to recursively split. Potentially that means branching the entire serialization state:

```c++
struct sector_state {
   std::vector<in_progress_function_pair> sector_functions;
};

// State for any given call stack frame in the recursive algorithm.
struct branched_sector_state {
   sector_state& common;
   std::vector<gw::expr::local_block> branches;
   size_t sector_id;
   size_t bits_remaining;
};

//
// When we encounter a union, we loop over its members and preemptively create 
// gw::expr::ternary instances, to generate the needed if/else trees in the 
// current and next sectors. We store these in a vector such that if we're at 
// sector A and about to split, then `blocks[x][A + y]` is the `local_block` 
// for the Xth union member's Yth branch.
//
// Then, we loop over each union member again. For each union member, we create 
// a copy of our current `branched_sector_state`. We set the copy's `branches` 
// list to `blocks[x]` and run it.
//
// Finally, after all union members are done, we take our `branched_sector_state` 
// and advance it to the end of the union.
//
```

We want to support two kinds of unions: *externally tagged unions* and *internally tagged unions*.

##### Externally tagged unions

An externally tagged union relies on a previous-sibling member to indicate the union's active member, as in the following example, where `Foo::tag` is the tag for `Foo::data`. We can support externally tagged unions by requiring that the union itself be annotated to indicate a previous-sibling member to serve as a tag, and requiring that each member of the union be annotated with its corresponding tag value. Notably, this would allow us to support situations where the tag values don't match the order of the union's members.

```c
#define LU_BP_TAG(name)    __attribute__((lu_bitpack_union_tag(name)))
#define LU_BP_TAG_VALUE(n) __attribute__((lu_bitpack_union_tagged_id(n)))

struct Foo {
   int tag = 2;
   LU_BP_TAG(tag) union {
      LU_BP_TAG_VALUE(0) struct {
         // ...
      } member_0;
      LU_BP_TAG_VALUE(1) struct {
         // ...
      } member_1;
      LU_BP_TAG_VALUE(2) struct {
         // ...
      } member_2;
   } data;
};
```

Requirements that we'll need to validate:

* Marking a freestanding union type as externally tagged is an error. However, it's fine to mark a freestanding union type's members as having tag values, and then mark individual data members of that type which appear inside of other structs.
* The union's tag member must be an integer type.
* The union's tag member must not be marked as omitted from bitpacking.
* The union's tag member must be a previous-sibling of the union. Probably the easiest way to verify this would be by comparing `offsetof`s; iterating over the containing struct's members isn't as likely to work due to the risk of missing anonymous structs.
* Each member of the union must be given a tag value, or must be marked as omitted from bitpacking. If any union member lacks a tag value and isn't marked as being omitted from bitpacking, then that's an error.
  * No two members of the union can have the same tag value.
  * All tag values assigned to members of the union must be representable within bitpacking. If we pack the tag such that the values [2, 7] can be serialized, then no union member may have a tag value like 1 or 8.

##### Internally tagged unions

An internally tagged union only contains structs as its members. All union members have the first *n* fields in common where *n* is at least 1, and one of these shared fields is the union tag.

```c
#define LU_BP_TAG_INTERNAL(name) __attribute__((lu_bitpack_union_internal_tag(name)))
#define LU_BP_TAG_VALUE(n)       __attribute__((lu_bitpack_union_tagged_id(n)))

LU_BP_TAG_INTERNAL(kind) union Foo {
   LU_BP_TAG_VALUE(0) struct {
      int   kind;
      int   divergent_0;
   } member_0;
   LU_BP_TAG_VALUE(1) struct {
      int   kind;
      float divergent_1;
   } member_1;
};
```

Requirements:

* Freestanding union types may be marked as internally tagged (if all other requirements are met) and, if so marked, may be used as top-level data values to serialize.
* All of the union's members must be of `struct` types.
* All of the union's members must have their own first *n* members be identical (same names, types, and if present, initializers) given some value of *n* > 0.
  * We may decide to only consider members that are not marked as omitted from bitpacking, such that omitted members may be interleaved between them.
* The identifier given as the union tag must be one of those *n* nested members.
  * That member must be an integer type.
  * That member must not be marked as omitted from bitpacking.
* Each member of the union must be given a tag value, or must be marked as omitted from bitpacking. If any union member lacks a tag value and isn't marked as being omitted from bitpacking, then that's an error.
  * No two members of the union can have the same tag value.
  * All tag values assigned to members of the union must be representable within bitpacking. If we pack the tag such that the values [2, 7] can be serialized, then no union member may have a tag value like 1 or 8.
