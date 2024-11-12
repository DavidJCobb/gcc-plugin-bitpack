
# Data options

The way any given data member is bitpacked will depend on bitpacking options specified in up to four places. Listed in ascending priority:

* Heritable options used[^heritable] by the member's (innermost)[^innermost] type
* Options applied to the member's innermost type
* Heritable options used by the member
* Options applied to the member

[^heritable]: `#pragma lu_bitpack heritable` can be used to define a reusable set of bitpacking options. Types and struct members can then refer to these via `__attribute__((lu_bitpack_inherit(name)))`.

[^innermost]: The "innermost" type of a given variable is the type with all array extents removed; for example, the innermost type of `int foo[5][4][3]` is `int`.

Accordingly, loading the bitpacking options is a bit... involved. The ultimate goal is to construct a `bitpacking::data_options::computed` instance, which holds all necessary data options in their most "fully realized" forms (e.g. resolving function names into `gw::decl::function` instances), coalescing the options specified in the above four sources.

## Loading

The loading process is carried out by `bitpacking::data_options::loader`.

We begin by creating two instances of `bitpacking::data_options::requested_via_attributes`: one for the member's type, and one for the member itself. A `requested_via_attributes` object handles parsing and examining a single declaration's attributes in isolation, reporting any errors. If no errors are reported, then we move on to coalescing the loaded options: combining the `requested_via_attributes` and requested heritables into a single set of `...::loader::coalescing_options`.

Once we've coalesced the options, we then apply any further computations, including but not limited to:

* If pre-pack and post-unpack transform functions have been specified, then we have to check whether both are present, and whether their arguments match.
* If the member to be serialized is an integral type, then we need to compute its bitcount if that wasn't explicitly specified. We compute the bitcount based on the member's minimum and maximum values and, if it's a C bitfield, its bit width. Note that the minimum and maximum values can be explicitly specified by the user (and are otherwise inferred from the member's type).