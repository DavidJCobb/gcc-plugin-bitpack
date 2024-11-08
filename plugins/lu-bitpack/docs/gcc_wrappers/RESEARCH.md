
# Things I might be able to research further

## Creating new `TYPE_DECL`s

Not sure what it takes to construct new `DECL`s generally.

After the `TYPE_DECL` is parsed and constructed, `TREE_TYPE(decl)` is the original type. The `set_underlying_type` in `c-family/c-common.h` is used to create a new `TYPE` named after the decl, setting `TREE_TYPE(decl)` to the new type and setting `DECL_ORIGINAL_TYPE(decl)` to the old type.

The `record_locally_defined_typedef` function records function-local typedefs for the sake of emitting warnings when they're unused.

