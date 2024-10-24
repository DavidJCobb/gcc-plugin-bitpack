
# To-do

* `handle_kv_string` should throw an exception with detailed error information for its caller to report
* Custom pragma to define a set of `HeritableOptions`, and a singleton to store them
  * Error on redefinition
  * Error on internally inconsistent options (e.g. string with a max integral value)
* `lu_bitpack_string` should error if used on something that isn't an array of [arrays of [...]] `char`.
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