
# To-do

## `lu-bitpack-rewrite`

### Short-term

C++:

* We should require and enforce that a union's tag be of an integral type (*exactly* an integral type; arrays of integrals should not be allowed).
* Codegen test-cases
  * Test for an edge-case: Internally tagged unions whose first members are identical at the language level but have different bitpacking options that would cause their sizes or serialized types to differ
* Investigate a change to transformations, to account for sector splitting. I want to allow the user to provide two kinds of transform functions. I don't know what to call them yet; the names here are purely temporary.
  * <dfn>Blind functions</dfn> work as transformation functions currently do, with respect to sector splitting: they must accept invalid data, have no way of knowing whether data is valid or invalid, and may be repeatedly invoked for "the same" object (at different stages of "construction") if that object is split across sectors.
    * Maybe call these "multi-stage" functions?
  * <dfn>Sighted functions</dfn> are only invoked on fully-constructed objects (i.e. an object that has been read from the bitstream in full), as is typical in programming generally. To ensure this, we'd define a `static` instance of each individual transformed object that gets split across sectors, so that we can invoke the post-unpack function only after an instance is fully read (and without needing to invoke the pre-pack function as a per-sector pre-process step for reads).
    * Maybe call these "one-stage" functions?
    * Sectors may be read out of order. Therefore, we would need one buffer for each transformed object that is split across sectors; where a nested transform is split, we need a buffer for each level of nesting. Each buffer would have to store a bitmask indicating which sectors (of those spanned by the transformed object) have already been read, with each such sector having code at its end to conditionally invoke the post-unpack function once we know the transformed object has been read in full.)
    
  This functionality would be fairly complex to implement, but the benefit it offers is that transform functions which require a complete object (e.g. because they want to assert correctness on read, or because the non-transformed data is the result of non-trivial calculations performed on the transformed data) will always receive one. Transform functions that don't require a complete object can be marked as "blind" and so run without needing to rely on static storage.
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
