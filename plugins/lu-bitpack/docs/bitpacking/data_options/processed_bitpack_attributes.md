
# `bitpacking::data_options::processed_bitpack_attributes`

There are three kinds of bitpacking options:

* **Global options**, specified via a pragma.
* **Heritable option sets ("heritables")**, each defined via a pragma.
* **Data options**, specified via attributes on types or field declarations.

When dealing with data options, there are two places we can do error checking: from the attribute handler, or during code generation. There are advantages to each:

Error checking from **attribute handlers** means that we validate bitpacking options for all types and values that specify them, even if those types or values aren't actually used within the list of to-be-serialized values. It also makes it easier to report *all* errors across all bad sets of options.

Error checking during **code generation** means that we validate bitpacking options only for types and values that we encounter while generating code for the list of to-be-serialized values. It also complicates error handling: because we build struct and member descriptors as we encounter to-be-serialized struct types, the easiest approach to error handling is to report errors on the first failing type or value, and then abort code generation immediately. In order to report errors on all (used) failing types or values, we'd have to pre-generate all descriptors before doing code generation.

## The two approaches to error checking and reporting

### During code generation

Code generation relies on "struct descriptors" and "member descriptors" that are generated as we encounter struct types to serialize. That is: we loop over all to-be-serialized values and, when necessary, recursively descend into their innards; and we create descriptors for struct types as we encounter them. Member descriptors store the computed bitpacking options for a given data member, coalescing heritable options and data options for the member's type and for the member itself. We coalesce those options &mdash; which is to say: we extract and process the relevant TYPE and DECL attributes &mdash; as part of building the member descriptor. This means that we handle error checking and reporting when building a member descriptor.

**Flaw:** Under this system, it is easiest to halt code generation immediately when any member descriptor has invalid options. If we want to report errors for all failing member descriptors, then we would have to pre-generate the descriptors before we actually start generating code, instead of spawning descriptors as needed.

**Flaw:** Under this system, we only perform error checking and reporting for types and fields that are actually included (directly or transitively) in the serialized output. If a type or field has data options but isn't used in the output, we never see it.

**Strength:** Under this system, all error checking can be done in a single place, without requiring global or heritable options to be specified before data options are seen.


### From attributes

We could theoretically validate most bitpacking options from within attribute handlers.

The naive approach would be to check for errors and report them within the attribute handlers, and then have codegen check for errors and fail silently afterward. However, this would mean that we'd have to duplicate all error checks across the two places; and it'd mean that codegen would still succeed if there are invalid attributes on unused types or fields.

An alternative approach is to have our attribute handlers synthesize a single `lu bitpack computed` attribute[^inaccessible-attr] which contains error-checked data options. The attribute's sole argument would be a `TREE_LIST` wherein key nodes are option names and value nodes are either `error_mark_node` or valid (e.g. an `INTEGER_CST` node for the `bitcount` option). We can then create a wrapper class such as `processed_bitpack_attributes` which provides convenient accessors for all data options, returning either "empty," "error," or a valid value.

[^inaccessible-attr]: If an attribute name has spaces in it, then it becomes impossible to use within source code; it can only exist if generated and applied by the compiler. GCC uses such attributes internally.

**Flaw:** Attribute handlers can only check for errors in the context of things that already exist by the time the attribute is seen. That is:

* In order to validate heritables (beyond rejecting non-strings and empty strings), we'd have to require that heritables be defined before they are referenced.
* In order to validate pre-pack and post-unpack functions, we'd have to require that they be declared before they are referenced by the relevant attributes.
* In order to fully validate options that can only be checked after coalescing is complete, we'd have to require that heritables be defined before they are referenced.
  * Example: a data member could specify its own pre-pack function yet inherit a post-unpack function; so we can only check if the functions' argument types match appropriately after coalescing is complete.
  * Example: we error if a string member is zero-length. However, `u8 name[1]` is considered zero-length if the string requires a null terminator in situ: we can only fully validate the length option in the context of the with-terminator option, and therefore we can only fully validate the length option after coalescing is complete.

One potential workaround for this is to have a "lightswitch" pragma: we start "lights-out," we only pay attention to attributes if the lights are turned on, and we only allow codegen if the lights were turned on before any data option attributes were seen[^lights-on]. The practical effect of this is: when adding bitpacking to an existing codebase, we could define heritable and global options, and declare any needed identifiers, in a single "serialization options" header, and only include that in the file that actually needs generated code.

[^lights-on]: We have to block codegen if any attributes were seen before the lightswitch was turned on. If we don't, then codegen won't validate those attributes (and, depending on implementation, may not even see them) and will produce incorrect results.

Another advantage of a "lightswitch" pragma is that it'd allow us to keep processing to a minimum. When the lights are out, we only need to keep track of whether we've seen any bitpacking attribute, but we don't need to actually track or remember anything about it.

**Strength:** Under this system, it is easiest to report errors for all invalid attributes across all failing types and values, rather than just the first type or value to fail.

**Strength:** Under this system, we'd perform error checking for all attributes, even if they exist on types or values that don't end up being included in the serialized output.

**Strength:** This would move the actual work of parsing the attribute out of the code to coalesce data options, and into the attribute handlers themselves: the handlers would do the work of, uh, handling the attribute and just provide a normalized result for codegen to consume.


## Implementation

