
# To-do

## `lu-bitpack-rewrite`

### Short-term

C++:

* GCC wrapper rewrite
  * NOTE: Given `typedef struct T {} U`, if both `T` and `U` appear in the to-be-serialized data, we emit separate `<struct>` elements for each of them in the XML output. This is because they are, technically, separate `..._TYPE` nodes.
    * I think this is a necessary evil. Typedefs can be annotated with bitpacking options separately from the original type, which can affect how they or their contents are bitpacked and therefore necessitate different bitpacking functions. In any case, this whole situation is a minor edge-case. We should probably document it somewhere (and document this paragraph i.e. the reason not to do anything about it) but otherwise leave it be.
  * Testcases needed:
    * XML output
      * Structs named via a typedef alone
      * Structs named via both a tag and a typedef
      * Transforms
        * Basic
        * Nested
        * Transitive
      * Tagged unions
        * External
        * Internal, defined in struct
        * Internal, defined and named separately
  * XML report generation feels a bit spaghetti -- specifically, the way we put XML tags together at the tail end of the process, when we "bake" output.
* Verify that our "on type finished" callback handler doesn't spuriously fire for forward-declarations. If it does, we do have a way to check if a type is complete, and we can gate things out based on that.
* Investigate a change to transformations, to account for sector splitting. I want to allow the user to provide two kinds of transform functions.
  * <dfn>Multi-stage functions</dfn> work as transformation functions currently do, with respect to sector splitting: they must accept invalid data, have no way of knowing whether data is valid or invalid, and may be repeatedly invoked for "the same" object (at different stages of "construction") if that object is split across sectors.
  * <dfn>Single-stage functions</dfn> are only invoked on fully-constructed objects (i.e. an object that has been read from the bitstream in full), as is typical in programming generally. To ensure this, we'd define a `static` instance of each individual transformed object that gets split across sectors, so that we can invoke the post-unpack function only after an instance is fully read (and without needing to invoke the pre-pack function as a per-sector pre-process step for reads).
    * Sectors may be read out of order. Therefore, we would need one buffer for each transformed object that is split across sectors; where a nested transform is split, we need a buffer for each level of nesting. Each buffer would have to store a bitmask indicating which sectors (of those spanned by the transformed object) have already been read, with each such sector having code at its end to conditionally invoke the post-unpack function once we know the transformed object has been read in full.
      * We'd have to have an intermediate step between generating an `instruction` tree and actually doing codegen, wherein we scan `transform` nodes (accounting for nesting) at the starts and ends of sectors and link any that have the same `to_be_transformed_value`: those nodes would need to be aware of each other (or at least aware of what sector they belong to and what sectors the to-be-transformed value spans).
    
  This functionality would be fairly complex to implement, but the benefit it offers is that transform functions which require a complete object (e.g. because they want to assert correctness on read, or because the non-transformed data is the result of non-trivial calculations performed on the transformed data) will always receive one. Transform functions that don't require a complete object can be marked as multi-stage and so run without needing to rely on static storage.
    * If any single-stage functions are used, then the caller of `generate_read` will need a way to manually signal that a read operation has started/completed/aborted, irrespective of what sectors are read first and last.

After we've gotten the redesign implemented, and our codegen is done, we should investigate using `gengtype` to mark our singletons as roots and ensure that tree nodes don't get deleted out from under our `basic_global_state`.
