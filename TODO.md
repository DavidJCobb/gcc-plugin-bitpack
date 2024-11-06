
# To-do

## `lu-bitpack`

### Short-term

* When loading attributes for a type, we should reject any that don't make sense for that type (e.g. integral options on a struct; string options on a non-array).
* Bitpacking options that make sense for struct types should appear in the XML output.
* When computing bitpacking options for a member, if pre-pack and post-unpack functions are specified, we need to verify that their "normal type" argument matches the innermost value type of the data member.
* The XML output has no way of knowing or reporting what bitpacking options are applied to integral types. The final used bitcounts (and other associated options) should still be emitted per member, but this still reflects a potential loss of information. Can we fix this?
  * Do we want to restrict bitpacking options to only be specifiable on struct types?
* Bitpacking options applied to a type are only detected when that exact type is used; given `typedef original_name new_name`, an object of type `new_name` will not benefit from any bitpacking options applied to `original_name`. Do we want to do something about this?
* Add an attribute that annotates a struct member with a default value. This should be written into any bitpack format XML we generate (so that upgrade tools know what to set the member to), and if the member is marked as do-not-serialize, then its value should be set to the default when reading bitpacked data to memory.
  * Most bitpacking options apply to the most deeply nested value when used on an array of any rank. For defaults, though, you'd need to at least be able to specify whether the value is per-element or for the entire member; and you'd then also need a syntax to provide values for (potentially nested) arrays. (We should support whatever initializer syntax C supports.)
* It'd be very nice if we could find a way to split buffers and strings across sector boundaries, for optimal space usage.
  * Buffers are basically just `void*` blobs that we `memcpy`. When `sector_functions_generator::_serialize_value_to_sector` reaches the "Handle indivisible values" case (preferably just above the code comment), we can check if the primitive to be serialized is a buffer and if so, manually split it via similar logic to arrays (start and length).
  * Strings require a little bit more work because we have to account for the null terminator and zero-filling: a string like "FOO" in a seven-character buffer should always load as "FOO\0\0\0\0"; we should never leave the tail bytes uninitialized. In order to split a string, we'd have to read the string as fragments (same logic as splitting an array) and then call a fix-up function after reading the string (where the fix-up function finds the first '\0', if there is one, and zero-fills the rest of the buffer; and if the string requires a null-terminator, then the fix-up function would write that as well). So basically, we'd need to have bitstream "string cap" functions for always-terminated versus optionally-terminated strings.
* Long-term plans below
  * Pre-pack and post-unpack functions
  * Union support

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

Other support requirements:

* When generating a to-be-bitpacked data member's computed bitpacking options, if the data member's type is a `struct`, then we must check for and apply `lu_bitpack_funcs` when it is present on the data member's type.
* We should rename `lu_bitpack_funcs` to `lu_bitpack_transform`.


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
