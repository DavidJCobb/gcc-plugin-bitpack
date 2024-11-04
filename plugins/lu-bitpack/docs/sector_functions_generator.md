
# Codegen

The general process is as follows:

* The user sets global bitpacking options using a pragma.
* The user annotates struct members with per-member bitpacking options using attributes.
* The user triggers generation of bitpacking functions using a pragma.

When the user actually triggers function generation, we handle that using `sector_functions_generator`. This takes the to-be-serialized variables (at translation unit scope) and generates several functions:

* Per-sector functions, of the form `void __(BitstreamStateType*)`.
* Whole-struct functions: whenever we determine that an entire struct of some type `T` can fit in the bitstream, we generate functions `void __lu_bitpack_read_T(BitstreamStateType*, T*)` and `void __lu_bitpack_save_T(BitstreamStateType*, const T*)` for it.

The `sector_functions_generator::run` function generates the per-sector functions, and generates whole-struct functions as needed. If a struct or array is positioned too close to the end of the current sector to fit (or if the object is too large to fit in a single sector at all), the generator will split the object across sector boundaries, generating code like the following:

```c
void __lu_bitpack_read_sector_0(struct lu_BitstreamState* state) {
   for(int i = 0; i <= 2; ++i) {
      __lu_bitpack_read_MyCoolStruct(state, &structArray[i]);
   }
   structArray[3].a = lu_BitstreamRead_u8(state, 3);
   structArray[3].b = lu_BitstreamRead_u8(state, 7);
}

void __lu_bitpack_read_sector_1(struct lu_BitstreamState* state) {
   structArray[3].c = lu_BitstreamRead_u8(state, 2);
   for(int i = 4; i <= 6; ++i) {
      __lu_bitpack_read_MyCoolStruct(state, &structArray[i]);
   }
}
```

To achieve this, we rely on a recursive function: `sector_functions_generator::_serialize_value_to_sector`. This function checks if the current value can fit, whole, in the current sector; if so, it generates the code to serialize it, and then returns. Otherwise, we branch based on the current value's type:

* If it's a struct, then we recurse on each of its members.
* If it's an array, then we loop until we've generated code to serialize the whole array:
  * Compute how many array elements can fit. If it's more than 1, then generate a for-loop for them.
  * Recurse on the next element.
* If it's indivisible, then move to the next sector and recurse on the object to try again.

Right now things are real noisy where I live, I have a splitting headache, and I can barely hear myself think, so I'm going to copy and paste the to-do list for this function's rewrite in here and if I never get around to writing a proper description, you can figure one out from that:

Accordingly, `_serialize_value_to_sector` is backed by four functions which are called when we know to a certainty that a given object will fit:

* `_serialize_whole_struct` returns an `expr_pair` of function calls to the appropriate whole-struct function. We lazy-create whole-struct functions as they're needed, and make sure to reuse them.
* `_serialize_array_slice` returns an `expr_pair` of for-loops to serialize a slice of an array (or the entire array).
* `_serialize_primitive` returns an `expr_pair` of function calls to serialize a single indivisible value.
* `_serialize` calls whichever of the three above functions is appropriate: it returns an `expr_pair` of code for serializing whatever's been handed to it.


## Struct and member descriptors

A *descriptor* captures relevant bitpacking information for a struct type or a struct member. Specifically, for each struct type, we create a `codegen::struct_descriptor` which stores a list of `codegen::member_descriptor`s, one per to-be-bitpacked member. The member descriptors store computed bitpacking options and other bitpacking-related information for the member.

To handle descent into (potentially nested) arrays, we mostly work with `codegen::member_descriptor_view`s when actually generating code.


## Serialization values

The `serialization_value` struct is a `value_pair` accompanied by a descriptor -- either for a top-level struct, or for a struct member. We use `serialiation_value`s for to-be-serialized data so that we always know where we are -- what struct descriptor or member descriptor view we're working with.