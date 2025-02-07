
# Uses of `decl_descriptor`

We need to extend the plug-in so that it can be asked to serialize certain kinds of expressions. For example, you may have a pointer variable and may wish to generate code to serialize not the pointer itself, but the pointed-to data, which you may not be able to refer to directly for a variety of reasons.

There are a few different ways we can handle this:

* Make the codegen system able to understand arbitrary values at most or all points of its execution
* Hack in the specific functionality we need: make it possible for `decl_descriptor` to indicate that it should be dereferenced *n* times prior to access, and allow specifying to-be-serialized variables as `*name`, `**name`, `***name` et cetera. There's not necessarily a need for more complex accesses (e.g. `name->member`) as once it's possible to dereference-and-serialize a pointer, it becomes possible to handle other accesses *using* a pointer: within the end user's code, declare a pointer `foo`, mark `*foo` it for serialization, and just set `foo = &name->member` before you call the generated serialization functions.
  * Would require adjusting `decl_descriptor` construction, as this would influence what we consider the in-memory and serialized types to be, and where we look for bitpacking options
  * Would require adjusting `codegen::value_path`, so that it knows how to generate the `gw::value` appropriately
    * Should `assert` that only the first-segment descriptor is marked for dereferencing. Perhaps the descriptor constructor can only allow dereferencing when describing a `VAR_DECL`, too.
  * We could potentially also use this to get rid of an ugly hack that we rely on for whole-struct functions. These functions take a struct instance pointer as an argument; in a few places, we assume that if we're describing a `PARM_DECL`, it's one of these pointers and needs to be deferenced. Having an explicit "dereference me!" marker on `decl_descriptor` could give us an "official" way to do this.
    * This special-case is currently handled in...
      * `codegen::decl_descriptor::decl_descriptor(gw::decl::param)`
      * `codegen::value_path::as_value_pair`
        * Assumes that if it's performing member or array access on a pointer, it must be one of these pointers; blindly dereferences it a single time. That is: if we see the path `a.b.c.d` but `a` is a pointer, then we assume that `a->b.c.d` is what was meant; ditto for `a[3].b.c.d` to `(*a)[3].b.c.d`. This would also affect the case of any non-trailing path segment being a variable e.g. `a.b.c.d` to `a.b.c->d`, but that shouldn't be possible.

## Things it does

* Serve as a unique identifier for a `DECL` (i.e. they can be identity-compared)
* Expose bitpacking options for a `DECL`
* Expose a `DECL`'s serialized type (as influenced by bitpacking options)
* Expose information about a `DECL`'s array extents (as interpreted based on bitpacking options)
* Expose descriptors for a struct- or union-type `DECL`'s to-be-serialized members
* Compute the serialized size in bits
* Compute the in-memory size

## Places where `decl_descriptor` is used

### Organized list

* Code generation (overall process)
  * Refer to the top-level variables which the user wants to generate serialization code for
  * Validate that the to-be-serialized variables, and all of their members, have valid bitpacking options
  * Generate whole-struct functions (we need descriptors to refer to the struct instance pointers that whole-struct functions take as arguments; we special-case these to dereference pointers on the assumption that this is the only time we'll handle `PARM_DECL`s)
* Serialization items
  * Segments
    * Check if we are an array
    * Return our array extent
    * Compute our serialized size in bits
    * Compute the serialized size in bits of one of our elements
  * Compute our serialized size in bits, and (if the value to which we refer is an array) the serialized size in bits of one of our elements (this functionality is handled by our segments)
  * Expose the descriptor of the value to which we refer
    * **TODO: What checks these?**
  * Expose the bitpacking options for the value to which we refer
    * **TODO: What checks these?**
  * Append a segment based on a descriptor (expansion/member access?)
  * Check whether we would influence the output in any way (i.e. the value to which we refer is not omitted, is omitted and defaulted, or is omitted and has defaulted members) based on the bitpacking options of the value to which we refer
  * Expansion
    * Check whether we can be expanded, based on the value to which we refer and its bitpacking options
    * Expand ourselves
* Serialization item list operations
  * Fold sequential array elements
    * Detect whether two consecutive serialization items refer to the same array, based on whether they refer to the same descriptor
    * Detect whether merging two consecutive serialization items would result in a whole-array array slice, based on the array information in their common descriptor
  * Force-expand omitted-and-defaulted values
    * Detect whether a serialization item refers to an omitted-and-defaulted value, or to an omitted value which has defaulted members
  * Force-expand unions and anonymous structs
    * Check the type of the value to which a serialization item refers
    * Check whether the value to which a serialization item refers is an array (to fully expand it if so)
* Rechunked items
  * All items
    * Constructor: expand serialization items that refer to arrays, i.e. make implied array slices explicit (given `int foo[4][3][2]`, an item for `foo` should produce the same chunks as `foo[0:4][0:3][0:2]`)
  * Qualified-decl chunks
    * Refer to a top-level value or to member access
* Value paths and segments thereof
  * Refer to a top-level variable
  * Refer to member access
  * Refer to array access using a variable index
  * Expose bitpacking options for the referred-to value
    * **TODO: What checks these?**
  * Generate `gw::value` nodes based on the member-access/array-access path
* Instruction nodes
  * Array slice
    * Refer to the created loop-index variable
  * Transform
    * Refer to created transformed-value variables
* XML output
  * Dump all members of a serialized struct, and their bitpacking options
  * Track created variables (loop indices, transformed values, etc.) so they can be given unique, human-intelligible names throughout the entire XML dump
  * Check the type and bitpacking options of a single serialized value, when generating XML for the generated instruction tree

### Raw list

* `decl_descriptor_pair`
* `decl_dictionary`
* `describe_and_check_decl_tree`
  * Checks if a `DECL` or any members thereof [or any members thereof [...]] has any errors (i.e. invalid data options for bitpacking).
* `codegen::instructions::array_slice`
  * A `decl_descriptor_pair` of the loop index variables (read and save), stored alongside the `gw::decl::variable`s themselves.
* `codegen::instructions::transform`
  * A `decl_descriptor_pair` for the final-transformed-type variables (read and save), stored alongside the `gw::decl::variable`s themselves.
  * Creates additional `decl_descriptor_pair`s for intermediate transform steps.
* `codegen::optional_value_pair`
  * Constructible from a `decl_descriptor_pair`, or from `gw::value`s or `gw::optional_value`s for read and save values.
* `codegen::rechunked::chunks::qualified_decl`
  * Stores a `std::vector<const decl_descriptor*>` to identify a variable or member thereof [or member thereof [...]].
* `codegen::serialization_item`
  * `...::append_segment(const decl_descriptor&)`
  * `...descriptor() const` returns a `const decl_descriptor&`
  * Needs to be able to access bitpacking options on its final segment in order to check whether the item can be expanded, and in order to perform the expansion. Needs to be able to access members of a described record/union in order to perform expansion as well.
  * Needs to be able to tell if its final segment refers to a `decl_descriptor` with any omitted-and-defaulted members. This is because an item which is omitted but not defaulted can still affect the output, if it contains omitted-and-defaulted members.
  * Needs to be able to report sizes in bits based on the descriptor (relies on `basic_segment` to actually do this).
* `codegen::serialization_items::basic_segment`
  * Stores a `decl_descriptor` pointer and some array access info.
  * Uses the decl descriptor to answer a few questions:
    * Is the current segment an array? If so, what's its extent?
    * What's the size in bits of the current segment, and if it's an array, what's the size in bits of a single element?
* `codegen::serialization_item_list_ops::fold_sequential_array_elements`
  * Given two consecutive serialization items, needs to be able to check if the items' last path segments both refer to the same entity (i.e. the same `DECL`). Compares the `decl_descriptor` pointers in the segments.
  * Given two consecutive serialization items that both refer to consecutive array accesses, needs to be able to check whether merging the two items together would result in a single array slice that covers the entire array. Given `int foo[3][2]`, a merge that produces `foo[0][0:2]` should simplify to `foo[0]`. This check requires knowing the array extent, which is pulled from the `decl_descriptor`.
* `codegen::serialization_item_list_ops::force_expand_omitted_and_defaulted`
  * Forcibly expands omitted serialization items if they are defaulted or contain any defaulted members. This requires being able to get the last segment, grab its descriptor, and call `desc.is_or_contains_defaulted()`.
* `codegen::serialization_item_list_ops::force_expand_unions_and_anonymous`
  * Relies on a serialization item's trailing segment's `decl_descriptor` to get the serialized value type; checks if this type is a union or an unnamed struct. Checks the descriptor's array extent, as well, to fully expand the serialiation item to its innermost array slice.
* `codegen::stats_gatherer`
  * Gathers stats for counts and whatnot, based on `decl_descriptor`s.
* `codegen::value_path_segment` and `codegen::value_path`
  * Wraps a `decl_descriptor_pair` indicating either a root variable, member access, or array access with a loop counter variable as the index. The descriptor can refer to the root variable or member being accessed, *or* to the loop counter.
  * Member access, and array access via a loop counter variable, are performed by passing in the descriptor of the member to access or the loop counter.
  * Value paths can be converted into pairs of `gw::optional_value`s (read and save) by starting at the first segment's `decl_descriptor`, and then following each successive segment's descriptors or non-variable array indices. In essence, we start at a `VAR_DECL` or `PARM_DECL` and we construct the usual GCC operations.
  * Bitpacking options are exposed via the "read" descriptor in the final non-array-access segment, if there is one.
* `pragma_handlers::generate_functions`
  * We assume that the to-be-serialized values are `VAR_DECL`s which can be described by `decl_descriptor`s.
* `codegen::report_generator`
  * Stores `codegen::decl_descriptor_pair`s for seen loop-counter variables and transformed variables, in order to give each such variable across the entire report a unique name and ID within the XML output.
  * When generating XML for an instruction to serialize a single entity, grabs the `decl_descriptor_pair` of the to-be-serialized entity. Gets the serialized type and bitpacking options from the descriptor.
* `codegen::instructions::utils::generation_context`
  * Uses `decl_descriptor`s when generating whole-struct functions, for the struct instance pointer argument.
* `codegen::rechunked::item`
  * The constructor has to expand arrays: given `int foo[4][3][2]`, a serialization item for `foo` should produce the same chunks as a serialization item for `foo[0:4][0:3][0:2]`. This is done by accessing the array extent information on a `codegen::serialization_items::basic_segment`'s wrapped `decl_descriptor`.
  * Additionally, items need to be able to report whether they're omitted-and-defaulted (why?), and they do this by checking bitpacking options on all of their `qualified_decl` chunks' descriptors.
* `codegen::rechunked::item_to_string`
  * Checks the `decl_descriptor`s wrapped by various chunks in order to stringify variable and member references.
* `xmlgen::referenceable_struct_members_to_xml`
  * When traversing a struct type's `FIELD_DECL`s, we blindly get-or-create `decl_descriptor`s for all of its members, so we can see and print their bitpacking options (if any).