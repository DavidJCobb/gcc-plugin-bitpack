
# To-do

## Short-term

C++:

* `lu-bitpack`
  * The project that I wanted to use this plug-in for requires that we be able to *dereference and then serialize* global pointers; i.e. we have some `gFooPtr` and we don't want to serialize the pointer itself, but rather we want to serialize the pointed-to data. The bitpacking plug-in currently can't do this, because it operates entirely in terms of `DECL`s.
    * We'd need to start by finding the places that rely on `decl_descriptor`s.
      * Serialization items consist of segments, which each refer to a `decl_descriptor`.
      * Rechunked items refer to `decl_descriptor`s via `qualified_decl` chunks.
      * Final code generation operates in terms of `value_path`s, which are defined in terms of `decl_descriptor_pair`s.
      * ...Is that everything?
    * Once we find all such places, we'd have to audit them and see if we could expand them to use arbitrary expressions. Among other things, we need to be able to ascertain what, exactly, each piece of code *needs* from the `decl_descriptor`s. Type or size information? Bitpacking options? Help with member access?
      * Notably, any member access into a value will lead to a `decl_descriptor` -- that of the `FIELD_DECL` being accessed.
      * My thinking is, we could create a `value_descriptor` type that wraps either a `decl_descriptor` or a value-expression, and have it offer the same information and capabilities that `decl_descriptor` presently offers. It may perhaps be easiest to define it as something akin to `std::variant<decl_descriptor, expression_descriptor>` so that the code for handling expressions is as clean as that for handling DECLs, with a class to glue them.
      * List of places
        * Serialization items
          * ...
        * Rechunked items
          * ...
        * Instruction nodes
          * ...
  * When we generate code for unions, we need to generate an `else` branch for when the tag value doesn't match any of the defined members. In this branch, we should zero-pad the entire union (minus any values we already read as part of handling an internally-tagged union).
  * Investigate being able to load global bitpacking options from an XML file (e.g. `#pragma lu_bitpack load_options "./path/to/file.xml"`), as a more convenient way to pass them in.
  * Versioning
    * The project builds, and testcase `codegen-various-a` passes at run-time, all the way up through GCC 14.2.0.
  * Verify that our "on type finished" callback handler doesn't spuriously fire for forward-declarations. If it does, we do have a way to check if a type is complete, and we can gate things out based on that.
  * Testcases needed:
    * Transforms
      * From non-union to internally tagged union
      * From union to internally tagged union

## Long-term

### Misc

* Fail if a union type is marked as an externally-tagged union. (Currently, we've marked the attribute as decl-only, but that causes attribute warnings emitted by GCC itself, not errors, much less errors that would prevent codegen.)
* XML report generation feels a bit spaghetti -- specifically, the way we put XML tags together at the tail end of the process, when we "bake" output.

### Transformations and sector splitting

* Investigate a change to transformations, to account for sector splitting. I want to allow the user to provide two kinds of transform functions.
  * <dfn>Multi-stage functions</dfn> work as transformation functions currently do, with respect to sector splitting: they must accept invalid data, have no way of knowing whether data is valid or invalid, and may be repeatedly invoked for "the same" object (at different stages of "construction") if that object is split across sectors.
  * <dfn>Single-stage functions</dfn> are only invoked on fully-constructed objects (i.e. an object that has been read from the bitstream in full), as is typical in programming generally. To ensure this, we'd define a `static` instance of each individual transformed object that gets split across sectors, so that we can invoke the post-unpack function only after an instance is fully read (and without needing to invoke the pre-pack function as a per-sector pre-process step for reads).
    * Sectors may be read out of order. Therefore, we would need one buffer for each transformed object that is split across sectors; where a nested transform is split, we need a buffer for each level of nesting. Each buffer would have to store a bitmask indicating which sectors (of those spanned by the transformed object) have already been read, with each such sector having code at its end to conditionally invoke the post-unpack function once we know the transformed object has been read in full.
      * We'd have to have an intermediate step between generating an `instruction` tree and actually doing codegen, wherein we scan `transform` nodes (accounting for nesting) at the starts and ends of sectors and link any that have the same `to_be_transformed_value`: those nodes would need to be aware of each other (or at least aware of what sector they belong to and what sectors the to-be-transformed value spans).
    
  This functionality would be fairly complex to implement, but the benefit it offers is that transform functions which require a complete object (e.g. because they want to assert correctness on read, or because the non-transformed data is the result of non-trivial calculations performed on the transformed data) will always receive one. Transform functions that don't require a complete object can be marked as multi-stage and so run without needing to rely on static storage.
    * If any single-stage functions are used, then the caller of `generate_read` will need a way to manually signal that a read operation has started/completed/aborted, irrespective of what sectors are read first and last.