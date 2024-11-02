
# Codegen

The general process is as follows:

* The user sets global bitpacking options using a pragma.
* The user annotates struct members with per-member bitpacking options using attributes.
* The user triggers generation of bitpacking functions using a pragma.

When the user actually triggers function generation, we handle that using `sector_functions_generator`. This takes the to-be-serialized variables (at translation unit scope) and generates several functions:

* Per-sector functions, of the form `void __(BitstreamStateType*)`.
* Whole-struct functions: whenever we determine that an entire struct of some type `T` can fit in the bitstream, we generate functions `void __lu_bitpack_read_T(BitstreamStateType*, T*)` and `void __lu_bitpack_save_T(BitstreamStateType*, const T*)` for it.

The `sector_functions_generator::run` function generates the per-sector functions, and generates whole-struct functions as needed. Right now things are real noisy where I live, I have a splitting headache, and I can barely hear myself think, so I'm going to copy and paste the to-do list for this function's rewrite in here and if I never get around to writing a proper description, you can figure one out from that:

* Recreate the top-level functionality of `sector_functions_generator` as a skeleton in the new project. Here are the elements we want to keep, in some form, from the failed draft:
  * The `expr_pair`, `func_pair`, and `value_pair` types. We want to generate the read and save functions alongside each other, rather than having to recursively traverse all data twice in order to generate the functions separately.
    * Accordingly, code-generation options should always run in tandem, producing or operating on these `*_pair` types. They should also offer tandem operations, e.g. `value_pair::access_member` returning another value pair.
    * Might be helpful to move these pair types to their own files, so there's less clutter in the `sector_functions_generator` files.
  * A dictionary which maps `gw::types` to struct type descriptions (`codegen::structure` in the failed draft). We should never generate a description for a to-be-serialized struct type more than once; when we find ourselves wanting to serialize a struct-type `gw::value`, we should check the dictionary and create a description only if the value does not exist.
  * The recursive loop in the `sector_functions_generator::run` function. This is what handles traversing over objects, deciding whether they can fit (in whole or in part) in the current sector, packing them or their subobjects into the current sector, splitting to the next sector, et cetera. The logic here is fine; I'm confident that it will work.
  * `sector_functions_generator::get_or_create_whole_struct_functions`, the function to generate whole-struct read/save functions on-demand given a `structure&`, though we may replace `structure&` with something else later (i.e. the redesigned "struct type description" class described above.
* What we need for `sector_functions_generator` next is:
  * The ability to recursively traverse objects and subobjects. In particular, it must be possible to recursively traverse an array and at all times know the array extent. I've already elaborated on this requirement above.
  * Three functions to generate an `expr_pair` for some object that we've already determined will fit in the current sector. These functions will be used both in sector generation (`...generator::run`) and when generating the whole-struct functions. These functions need to be given the `value_pair` for the bitstream pointer, the `value_pair` for the to-be-serialized object, and the "member description" for the to-be-serialized object.
    * One for by generating calls to the serialize-whole-struct functions (and triggering creation of the functions themselves if they do not exist).
    * One for serializing a non-struct non-array (a "primitive").
    * One for serializing an array (or a slice thereof; this function will also be used for when we know an array slice will fit).
      * If it's an array of arrays, the function should recurse.
      * If it's an array of structs, the function should, for each element, call the function to generate a "serialize whole struct" `expr_pair`.
      * Otherwise, the function should, for each element, call the function to generate a "serialize primitive" `expr_pair`.


## Struct and member descriptors

A *descriptor* captures relevant bitpacking information for a struct type or a struct member. Specifically, for each struct type, we create a `codegen::struct_descriptor` which stores a list of `codegen::member_descriptor`s, one per to-be-bitpacked member. The member descriptors store computed bitpacking options and other bitpacking-related information for the member.

To handle descent into (potentially nested) arrays, we mostly work with `codegen::member_descriptor_view`s when actually generating code.


## Serialization values

The `serialization_value` struct is a `value_pair` accompanied by a descriptor -- either for a top-level struct, or for a struct member. We use `serialiation_value`s for to-be-serialized data so that we always know where we are -- what struct descriptor or member descriptor view we're working with.