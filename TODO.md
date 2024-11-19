
# To-do

## `lu-bitpack-rewrite`

### Short-term

C++:

* We should require and enforce that a union's tag be of an integral type (*exactly* an integral type; arrays of integrals should not be allowed).
* Codegen test-cases
  * Internally tagged unions
    * As struct members
    * As top-level VAR_DECLs
  * Transformed objects
    * **Done:** A single transform (A -> B)
    * A transitive transform (A -> B -> C)
    * A nested transform (the transformed type contains a to-be-transformed type)
  * Test for an edge-case: Internally tagged unions whose first members are identical at the language level but have different bitpacking options that would cause their sizes or serialized types to differ
* XML output
* Statistics logging

After we've gotten the redesign implemented, and our codegen is done, we should investigate using `gengtype` to mark our singletons as roots and ensure that tree nodes don't get deleted out from under our `basic_global_state`.


### Future features

#### Statistics

The XML output should include the information needed to generate reports and statistics on space usage and savings.

* Every C bitfield should be annotated with its C bitfield width.
* Every type element (e.g. `struct-type`) should report the following stats:
  * Number of instances seen in the serialized output
    * Per top-level identifier
    * Total
  * `sizeof`
  * `serialized-bitcount`
  * If the type is an array type (could happen if an array typedef has bitpacking options), then its array ranks.
  * If the type originates from a typedef, then it should have an `is-typedef-of` attribute listing the name of the nearest type worth outputting an XML element for or, if none are worth it, the end of the transitive typedef chain.
* If a type is annotated with `__attribute__((lu_bitpack_track_type_stats))`, then we should guarantee that the XML output will contain an element representing that type. If it's not a struct, then it should get an appropriate tagname (even just `type` would do).

Accordingly, type XML should reformatted:

```xml
<!-- `general-type` or `struct-type` -->
<general-type name="..." c-type="..." sizeof="..." serialized-bitcount="...">
   <array-rank extent="1" /> <!-- optional; repeatable -->
   
   <!-- bitpack options listed as attributes here, incl. x-options type -->
   <options />
   
   <fields>
      <!-- ... -->
   </fields>
   <stats>
      <times-seen total="5">
         <per-top-level-identifier identifier="foo" count="3" />
         <!-- if the type IS the type of a top-level identifer, then consider that a count of 1 for that identifier -->
      </times-seen>
   </stats>
</general-type>
```

Goal:

```c
#define LU_BP_MAX_ONLY(n) __attribute__((lu_bitpack_range(0,n)))
#define LU_BP_RANGE(x,y)  __attribute__((lu_bitpack_range(x,y)))
#define LU_BP_TRACK       __attribute__((lu_bitpack_track_statistics))

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

LU_BP_TRACK LU_BP_MAX_ONLY(LANG_COUNT) typedef uint8_t language_t;

LU_BP_TRACK LU_BP_STRING_UT typedef char player_name_t [7];
```
