
# To-do

## `lu-bitpack`

Currently writing the bitpack logic in `bitpacking/sctor_functions_generator.(h|cpp)`.

**Old bitpacking approach:**

This is the approach I used to generate C source code from XML struct definitions.

* Index all types and their members' bitpacking options (load XML definitions)
* For each struct type, generate a function that will serialize an entire instance of that type.
* Create a list of `serialization_item` instances (the "item list") &mdash; one for each top-level struct to serialize.
  * Each `serialization_item` consists of a pointer to an indexed struct, a pointer to an indexed struct member, a list of zero or more array indices, and a string representing the fully-qualified path to the described member.
* For each item in the item list:
  * Try to fit the item into the current sector. (In particular, if it's a struct type or an array thereof, then we invoke one of the functions we generated earlier to serialize the thing whole.)
  * If the item doesn't fit, then try to "expand" it.
    * If expansion is possible (i.e. if the item is a struct or array), then it will produce multiple `serialization_item` instances; replace the current item with those (splicing them into the item list), and then retry from the first of them.
      * For example, given a member `int foo[3][2][3]`, expanding `foo` produces `foo[0]`, `foo[1]`, and `foo[2]`; and expanding `foo[0]` produces `foo[0][0]` and `foo[0][1]`.
      * Similarly, given a member `struct foo_type { int a; int b; } c[2]`, expanding `c[0]` produces `c[0].a` and `c[0].b`. Both of these serialization items refer to the `foo_type` struct and its members `a` and `b`; the only way to know that we got here from `c[0]` is via the path string (e.g. `"c[0].a"`).
    * If expansion is not possible, then start a new sector.

As you can imagine, the use of string paths here complicates using this approach in our GCC plug-in.

**New bitpacking approach:**

* Start the zeroth sector.
* For each top-level variable to serialize:
  * **[A]** For each member of the current variable:
    * If the member is a struct, then...
      * If the member can fit entirely in the current sector, then bitpack it whole.
      * Else if the member can't fit, then...
        * Recursively execute **[A]** treating this member as the new current variable.
    * Else if the member is a non-struct or array of structs, then...
      * If the member can fit entirely in the current sector, then bitpack it whole.
      * Else if the member can't fit, then...
        * If the member isn't an array, then start a new sector, and then recursively execute **[A]** treating the member as the new current variable.
        * Else if the member is an array...
          * While there are any array elements left to serialize:
            * Compute the number of elements *n* that can fit in the current sector's available space.
            * Bitpack the next *n* elements.
            * Recursively execute **[A]** treating the next element as the new current variable. The effect of this will be to split the element across sector boundaries if indeed it can be split, and start a new sector either way.

Rather than indexing the bitpacking options and data for all structs in advance, we instead extract those options from struct types as those types are encountered, caching them for fast retrieval should we encounter more instances of the same struct type later.

Similarly, we don't split structs across sector boundaries (with the `serialization_item` lists) in advance of code generation. Instead, we perform the sector splits as we generate code.

In essence, we've folded three passes into one: we index bitpacking options and handle sector splitting as we generate the code.

### Details

The process is managed by a `sector_functions_generator` object. The object generates the per-sector read and save functions alongside each other, which among other things means that instead of operating directly in terms of GCC wrappers (e.g. `gw::value`), it has to use bifurcated data (e.g. `sector_functions_generator::target`, which stores one `value` for the "read" half and one value for the "save" half).

### To-do

* In general we should be caching the `gw::type` wrappers around built-in types like `uint8_type_node`, since `gw::type::from_untyped` verifies the `TREE_CODE` and we shouldn't be redundantly doing that a million times per function we generate. In fact, it'd probably be best to have a singleton that just holds `gw::type`s for built-ins.
* There's no way to communicate global bitpacking options (e.g. sector size, sector count, sector layouts) to the `sector_functions_generator`.
* We need code to generate whole-struct bitstream-read and bitstream-write functions. They should be generated on first use.
* When dealing with an array of arrays, `sector_functions_generator::in_progress_func_pair::serialize_array_slice` needs to be able to run recursively.
* When dealing with an array of structs, `sector_functions_generator::in_progress_func_pair::serialize_array_slice` needs code to generate the appropriate calls to the whole-struct functions we generated.
* `sector_functions_generator::in_progress_func_pair::serialize_entire` is unimplemented.
* `sector_functions_generator::in_progress_sector` needs a constructor which creates the initial read/save funcs and their root blocks.
* `sector_functions_generator::in_progress_sector::next` needs to create the next set of read/save funcs and their root blocks.
* Once we have the `sector_functions_generator` working on a basic level, we'll then want to implement the "layout" pragma, which gives you "fuzzy" control over which structs go in which sectors (i.e. being able to say, "*this* struct right here should get its own sector").
  * Perhaps we should implement an alternate syntax when specifying the variables to serialize, e.g. `( a | b c | d )` to indicate, for example, that after `a` we should skip to the start of the next sector, and after `c` we should skip to the next sector, such that `b` and `c` can potentially share a sector, but `a` never shares a sector with `b`, nor `c` with `d`.

## outdated; review

* `handle_kv_string` should throw an exception with detailed error information for its caller to report
* Custom pragma to define a set of `HeritableOptions`, and a singleton to store them
  * Error on redefinition
  * Error on internally inconsistent options (e.g. string with a max integral value)
* If `lu_bitpack_string` specifies a string length, it should error if the deepest rank of the char array it's used on is not of that length (e.g. if the length is 7, `char foo[3][7]` should be accepted and `char foo[3][5]` should fail).
* Finish implementing `lu_bitpack_inherit` for having a struct member use a set of heritable options
* Finish implementing `lu_bitpack_funcs` (parsing only)
* Add an attribute that annotates a struct or a struct member as being `memcpy`'d into the bitpacked stream. (Actually, it'd be bitpacked one byte at a time, so as to avoid padding bits around the nenber; but the member itself would be treated as an opaque blob of data.)
* Add an attribute that annotates a struct member with a default value. This should be written into any bitpack format XML we generate (so that upgrade tools know what to set the member to), and if the member is marked as do-not-serialize, then its value should be set to the default when reading bitpacked data to memory.
* Implement annotation and processing of unions
  * If a union's members are all structs, and each struct's first member is of the same type, alignment, and name, then allow that member to be used as the union's tag (but require that the user specify this, and require that they indicate which values map to which inner structs).
* Add pragmas to set optional top-level serialization params.
  * Sector size
  * Max sector count
  * Max total size
  * Version number location (start of sector 0; start of all sectors; no version number)
* Add a pragma which allows you to specify that a given typedef (e.g. `BOOL`, `bool8`, etc.) always refers to a bool and should by default serialize as a single bit based on whether its value is non-zero. Try to catch nested typedefs (e.g. `typedef BOOL smeckledorfed; smeckledorfed spongebob = TRUE;`) in the struct-/union-handling logic.
* Add pragmas to specify the names of functions that can serialize various lowest-level data types, as well as a pragma to specify a "state" data type that these functions should take as an argument.
  * "Read" functions for integrals should take a state pointer and a bitcount, and return a result. It should be possible to specify different functions for different in-memory sizes (i.e. 8-bit, 16-bit, 32-bit). If the largest serialized value has an *n*-bit in-memory type, you should not need a function that can handle larger in-memory sizes.
  * "Read" functions for strings should take a state pointer, a destination string `uint8_t` pointer, and a max length. There should be separate functions for strings that require an in-memory null terminator and strings that do not.
  * "Read" functions for bools should take a state pointer.
  * "Write" functions for integrals should take a state pointer, value, and bitcount.
  * "Write" functions for strings should take a state pointer, source string `uint8_t` pointer, and a max length. There should be separate functions for strings that require an in-memory null terminator and strings that do not.
  * "Write" functions for bools should take a state pointer and a value.
  * "Read" and "write" functions for arbitrary buffers should take the same basic arguments as strings, but using void pointers rather than `uint8_t` pointers. These functions should only be necessary if any structs or struct members are annotated as being directly `memcpy`'d.
* Add a pragma which generates the code to serialize a given variable (e.g. a global instance of some struct) into the bitpacked data, continuing from where serialization left off (or starting at the beginning of sector 0 if it's the first thing we're generating). For now, we only care about serializnig globals, so reading/writing directly from/to them is fine.
  * When serialization is divided into sectors, there should be a general entry point that takes a sector index and a source/destination buffer. When serialization is not divided into sectors, just take a buffer.
  * Start with the basic data types first; then implement the pre-pack and post-unpack function option.
* Add a pragma which forces serialization to skip to the start of the next sector, leaving any remaining space in the current sector unused.