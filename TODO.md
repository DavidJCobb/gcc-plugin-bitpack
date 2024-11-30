
# To-do

## `lu-bitpack-rewrite`

### Short-term

C++:

* GCC wrapper rewrite
  * Rename `node::as_raw` to `node::unwrap` and update the already-ported code to match.
  * `list` shouldn't be a view-like wrapper. Make it a reference-like wrapper.
  * Consider alternate names for reference-like, pointer-like, and view-like wrappers.
    * Valued, nullable, and view? Naming convention could be `node` versus `node_nb`, and getting rid of pointers-as-allegories means we can get rid of `operator->` and the casting shenanigans it requires.
  * Set up a small plug-in that allows us to just dump info about various identifiers using the wrappers. Treat it as a unit test. Look, also, into writing helper functions for things that are currently done in `lu-bitpack-rewrite`'s plug-in-specific codegen (e.g. printing the signature of a `FUNCTION_DECL`, as is done in `generate_functions.cpp` when we report problems with finding builtins).
  * Once we've proofed the basic wrapper functionality, start work on porting `lu-bitpack-rewrite` to `lu-bitpack-rewrite-2`. Try to do so in a modular way: focus on the attribute handlers and the code to load global settings; then the rest of codegen. We should try to avoid getting ourselves in a situation where we have to port a couple dozen files before we can build again, even if that means straight-up *rebuilding* the codegen and settings and whatnot instead of merely porting them.
* Verify that our "on type finished" callback handler doesn't spuriously fire for forward-declarations. If it does, we do have a way to check if a type is complete, and we can gate things out based on that.
* Look into renaming `lu::strings::printf_string` to something else, since it uses `printf`-style syntax but doesn't actually print anything. I don't want to burn the name "format" since that would be better-suited to something that polyfills `std::format`, should I ever bother to do that. Perhaps `lu::stringf`?
* Work on rewriting the GCC wrappers to distinguish between pointer-style wrappers and reference-style wrappers. Do that separately and in its own folder; then gradually bring pieces of the plug-in over to rewrite.
* Delete `lu-plugin-bitpack` and all other outdated copies of the plug-in; have just the main plug-in.
* Investigate removing `bitpacking::member_kind::transformed`. We now treat transformation as an operation to be applied "around" values (akin to for loops), rather than as something to be applied per value.
  * It's set on a `decl_descriptor` if the computed options are transform options, but I'm not sure it's checked anywhere. We'd have to check for what code reads the member-kind generally, in case any value we might want to use instead (e.g. `none`) might alter behavior.
  * Maybe we want to replace this with something like `value_kind`, or do a better job of integrating it with X-options so that the options `variant` it's associated with just... also acts as the tag.
* Investigate a change to transformations, to account for sector splitting. I want to allow the user to provide two kinds of transform functions.
  * <dfn>Multi-stage functions</dfn> work as transformation functions currently do, with respect to sector splitting: they must accept invalid data, have no way of knowing whether data is valid or invalid, and may be repeatedly invoked for "the same" object (at different stages of "construction") if that object is split across sectors.
  * <dfn>Single-stage functions</dfn> are only invoked on fully-constructed objects (i.e. an object that has been read from the bitstream in full), as is typical in programming generally. To ensure this, we'd define a `static` instance of each individual transformed object that gets split across sectors, so that we can invoke the post-unpack function only after an instance is fully read (and without needing to invoke the pre-pack function as a per-sector pre-process step for reads).
    * Sectors may be read out of order. Therefore, we would need one buffer for each transformed object that is split across sectors; where a nested transform is split, we need a buffer for each level of nesting. Each buffer would have to store a bitmask indicating which sectors (of those spanned by the transformed object) have already been read, with each such sector having code at its end to conditionally invoke the post-unpack function once we know the transformed object has been read in full.
      * We'd have to have an intermediate step between generating an `instruction` tree and actually doing codegen, wherein we scan `transform` nodes (accounting for nesting) at the starts and ends of sectors and link any that have the same `to_be_transformed_value`: those nodes would need to be aware of each other (or at least aware of what sector they belong to and what sectors the to-be-transformed value spans).
    
  This functionality would be fairly complex to implement, but the benefit it offers is that transform functions which require a complete object (e.g. because they want to assert correctness on read, or because the non-transformed data is the result of non-trivial calculations performed on the transformed data) will always receive one. Transform functions that don't require a complete object can be marked as multi-stage and so run without needing to rely on static storage.
    * If any single-stage functions are used, then the caller of `generate_read` will need a way to manually signal that a read operation has started/completed/aborted, irrespective of what sectors are read first and last.

After we've gotten the redesign implemented, and our codegen is done, we should investigate using `gengtype` to mark our singletons as roots and ensure that tree nodes don't get deleted out from under our `basic_global_state`.
