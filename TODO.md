
# To-do

## `lu-bitpack-rewrite`

### Short-term

C++:

* We should require and enforce that a union's tag be of an integral type (*exactly* an integral type; arrays of integrals should not be allowed).
* Regression: when converting a re-chunked item list to nodes, conditions aren't folded to share `union_switch` nodes. See `generation_a` testcase with unions and arrays enabled.
* Time for codegen.
  * Spawn a `whole_struct_function_dictionary`, and then, for each sector:
    * Create the read and save `gw::decl::function`s, each with a local bitstream-state variable. Generate the "initialize" call in each.
    * Create a `value_pair` wrapping `state_decl.as_value().address_of()` from the read and save functions. Write that value pair and a reference to your dictionary into a new `instruction_generation_context`.
    * Create an instruction node from the sector's serialization item list.
    * Call `root->generate(context)` to get an `expr_pair`. This *may* be a pair of `gw::expr::local_block` instances; if it isn't, then wrap it in them. (See near the end of `instruction_generation_context::_make_whole_struct_functions_for` for an example.)
    * Set the local blocks as the root blocks of the created sector function.

After we've gotten the redesign implemented, and our codegen is done, we should investigate using `gengtype` to mark our singletons as roots and ensure that tree nodes don't get deleted out from under our `basic_global_state`.


### Need another friggin' redesign lmao

(IN PROGRESS. TO-DO LIST ABOVE IS THE CURRENT TASKS FOR THIS.)

The way we group conditions within `serialization_item`s is not viable. It breaks down when a condition is nested inside of a transformation (i.e. a transformed type contains a union, and may need to be split across sectors):

```c
struct Transformed_A {
   int tag;
   union {
      int a;
   } data;
};

struct TestStruct {
   int tag;
   union {
      struct ToBeTransformed_A a;
   } data;
} sTestStruct;
```

The generated code would be akin to:

```c
sTestStruct.tag;
if (sTestStruct.tag == 0) {
   {
      Transformed_A __transformed_a;
      __transform(&sTestStruct.data.a, &__transformed_a);
      __transformed_a.tag;
      if (__transformed_a.tag == 0) {
         __transformed_a.data.a;
      }
   }
}
```

We need an alternate representation for serialized items:

```
sTestStruct|tag
sTestStruct|(tag == 0) data|a as Transformed_A
sTestStruct|(tag == 0) data|a as Transformed_A|tag
sTestStruct|(tag == 0) data|a as Transformed_A|(tag == 0)|data|a
```

Conclusions so far:

* Each segment can have one condition.
* If a segment is transformed, then the next segment is a member of the previous segment's transformed value.

How should arrays work?

```c
struct TestStruct {
   int a[5];
   int tag;
   union {
      int a;
      int b;
      int c;
   } data[2];
   int footer[3][2];
} sTestStruct;
```

This should produce serialization items that look like this when fully expanded:

```
sTestStruct|a[0]
sTestStruct|a[1]
sTestStruct|a[2]
sTestStruct|a[3]
sTestStruct|a[4]
sTestStruct|tag
sTestStruct|data[0]|(sTestStruct.tag == 0) a
sTestStruct|data[0]|(sTestStruct.tag == 1) b
sTestStruct|data[0]|(sTestStruct.tag == 2) c
sTestStruct|data[1]|(sTestStruct.tag == 0) a
sTestStruct|data[1]|(sTestStruct.tag == 1) b
sTestStruct|data[1]|(sTestStruct.tag == 2) c
sTestStruct|footer[0][0]
sTestStruct|footer[0][1]
sTestStruct|footer[1][0]
sTestStruct|footer[1][1]
sTestStruct|footer[2][0]
sTestStruct|footer[2][1]
```

If `data|[n]` were not expanded[^cant-unexpand], then the array slices could coalesce to the following.

```
sTestStruct|a[0:5]
sTestStruct|tag
sTestStruct|data[0:2]|(sTestStruct.tag == 0) a
sTestStruct|data[0:2]|(sTestStruct.tag == 1) b
sTestStruct|data[0:2]|(sTestStruct.tag == 2) c
sTestStruct|footer[0:3][0:2]
```

[^cant-unexpand]: Given an array of unions, once the unions are expanded into individual union members, it becomes impossible to coalesce the array-of-unions back into contiguous slices without doing very tricky look-aheads that require knowledge of what a full element looks like. In general, expansion is a "one-way" operation, and coalescing an expanded array into array slices is a special-case operation in the reverse direction.

So the syntax that we want for serialization items, then, is:

```c++
class serialization_item {
   public:
      class segment {
         struct {
            const decl_descriptor* lhs = nullptr; // relative to previous non-array segment.
            intmax_t               rhs = 0;
         } condition;
         
         const decl_descriptor* desc = nullptr;
         std::vector<array_access_info> array_accesses;
         std::vector<gw::type::base> transformations;
      };
   
   public:
      std::vector<segment> segments;
};
```

The string syntax for serialization items becomes:

```
serialization-item → segment ('|' segment)*

   segment → condition? decl array-access* transformation*
   
      condition → '(' fully-qualified-decl " == " integer-constant ") "
      
         fully-qualified-decl → fully-qualified-decl-segment+
         
            fully-qualified-decl-segment → decl array-access*
      
      decl → VAR_DECL | PARM_DECL | FIELD_DECL
      
      array-access → array-access-single | array-access-multiple
      
         array-access-single → '[' integer-constant ']'
         
         array-access-multiple → '[' integer-constant ':' integer-constant ']'
      
      transformation → " as " typename
```

Notation:

* `rule-name → matched-content`
* quotation marks indicate char and string literal content to be matched
* `foo*` means at least zero `foo`
* `foo+` means at least one `foo`
* `foo?` means zero or one `foo`
* vertical bars are an "OR" operator
* parentheses are a grouping operator

#### Putting it all together

Consider this code:

```c
struct Transformed_E {
   int a;
   int b;
};
struct Transformed_F {
   int x;
};
struct Transformed_Z {
   int tag;
   union {
      int a;
      int b;
   } data;
};

struct ToBeTransformed_E {
   // empty
};
struct ToBeTransformed_Z {
   // empty
};

static struct TestStruct {
   int a;
   int b;
   int c[3];
   int d[3][2];
   struct ToBeTransformed_E e;
   struct ToBeTransformed_F f[2];
   
   int tag_g;
   union {
      int a;
      struct {
         int tag;
         union {
            int a;
         } data;
      } b;
   } data_g;
   
   int tag_x;
   union {
      int a;
      int b;
      int c;
   } data_x;
   int tag_y;
   union {
      int a;
      int b;
      int c;
   } data_y[2];
   struct ToBeTransformed_Z z;
   struct ToBeTransformed_Z z_array[2];
} sTestStruct;
```

The fully expanded serialization items would be:

```
sTestStruct|a
sTestStruct|b
sTestStruct|c[0]
sTestStruct|c[1]
sTestStruct|c[2]
sTestStruct|d[0][0]
sTestStruct|d[0][1]
sTestStruct|d[1][0]
sTestStruct|d[1][1]
sTestStruct|d[2][0]
sTestStruct|d[2][1]
sTestStruct|e as Transformed_E|a
sTestStruct|e as Transformed_E|b
sTestStruct|f[0] as Transformed_F|x
sTestStruct|f[1] as Transformed_F|x
sTestStruct|tag_g
sTestStruct|data_g|(sTestStruct.tag_g == 0) a
sTestStruct|data_g|(sTestStruct.tag_g == 1) b|tag
sTestStruct|data_g|(sTestStruct.tag_g == 1) b|data|(sTestStruct.data_g.b.tag == 0) a
sTestStruct|tag_x
sTestStruct|data_x|(sTestStruct.tag_x == 0) a
sTestStruct|data_x|(sTestStruct.tag_x == 1) b
sTestStruct|data_x|(sTestStruct.tag_x == 2) c
sTestStruct|tag_y
sTestStruct|data_y[0]|(sTestStruct.tag_y == 0) a
sTestStruct|data_y[0]|(sTestStruct.tag_y == 1) b
sTestStruct|data_y[0]|(sTestStruct.tag_y == 2) c
sTestStruct|data_y[1]|(sTestStruct.tag_y == 0) a
sTestStruct|data_y[1]|(sTestStruct.tag_y == 1) b
sTestStruct|data_y[1]|(sTestStruct.tag_y == 2) c
sTestStruct|z as Transformed_Z|tag
sTestStruct|z as Transformed_Z|data|(sTestStruct.z.tag == 0) a
sTestStruct|z as Transformed_Z|data|(sTestStruct.z.tag == 0) b
sTestStruct|z[0] as Transformed_Z|tag
sTestStruct|z[0] as Transformed_Z|data|(sTestStruct.z[0].tag == 0) a
sTestStruct|z[0] as Transformed_Z|data|(sTestStruct.z[0].tag == 1) b
sTestStruct|z[1] as Transformed_Z|tag
sTestStruct|z[1] as Transformed_Z|data|(sTestStruct.z[1].tag == 0) a
sTestStruct|z[1] as Transformed_Z|data|(sTestStruct.z[1].tag == 1) b
```

Details worth noting:

* Given a condition `foo.bar.baz`, if `bar` is a transformed value, then `baz` is a member of the final transformed type.
* Conditions need to be able to access qualified names, including with array indices, because the conditional operand may be a sibling of the union member (for externally tagged unions) *or* it may be inside of the union (for internally tagged unions). The syntax `(tag == 0) union_member|a` would allow us to have unqualified names for externally-tagged unions, but wouldn't free us of the need for a qualified name when dealing with internally tagged unions' members: `union_member|(a.tag == 0) b`.

### Short-term

Unions; then codegen. I don't want to inadvertently implement codegen in ways that lock me out of doing unions in the future, so let's get union serialization items working first; and when we know we've got everything ready on the serialization item and sector splitting end of things, we can then work on codegen.

* `codegen::decl_descriptor`: If computed options fail to load, we should throw `std::runtime_error`.
* `pragma_handlers::debug_dump_as_serialization_item`: We should catch a `std::runtime_error` which may be thrown when attempting to retrieve a descriptor.
* Implement code generation wherein each sector is based on a flat list of `serialization_item`s.
  * Where two serialization items refer to data located within the same transformed object, transform the object only once and take care of both serialization items then. (Basically: if the two paths have the same stem, handle them together.)
  * Where two serialization items have the same conditions, handle them together. Account for cases of one item's conditions being a superset of the other (i.e. nested if/else). Unions, and nested unions, are the test-case for this.
* After codegen is done, we need to implement XML output again. It'll be a bit easier this time, since the same lists of `serialization_item`s that we use to generate the functions (whole-struct and per-sector) should also have sufficient information to generate the XML.
* Devise a test-case for top-level integral, buffer, and string values.
* Devise a test-case for top-level internally tagged unions.
* Devise a test-case for top-level arrays (of each type: integral, buffer, string, internally tagged union, and struct).
* After codegen is done, we need to implement stat tracking. It should be possible to annotate a type or declaration with `__attribute__((lu_bitpack_stats_category("foo")))` such that we know how many "foo" have made it into the serialized bitstream. We should also track the in-memory size (`sizeof`) versus the serialized size of "foo."

### Code generation

Example input struct (assume all fields are bitpacked; assume union members have sequential IDs):

```c
struct {
   int tag;
   union {
      struct {
         int a;
         int b;
      } permutation_0;
      struct {
         int a;
      } permutation_1;
      struct {
         int tag;
         union {
            int permutation_x;
            int permutation_y;
         } data;
      } permutation_2;
      struct {
         int a;
      } permutation_3;
   } data;
   int b;
   struct ToBeTransformed c;
   struct ToBeTransformed d;
      // .member_u
      // .member_v
      // ((ToBeTransformed) .member_w).innermost
      // .member_x
   int e;
   struct ToBeTransitivelyTransformed f;
   int g; // tag for h
   union {
      int permutation_0;
   } h[5];
} sTestStruct;
```

Serialization items:

```
sTestStruct.tag
if (sTestStruct.tag == 0) sTestStruct.data.permutation_0.a
if (sTestStruct.tag == 0) sTestStruct.data.permutation_0.b
if (sTestStruct.tag == 1) sTestStruct.data.permutation_1.a
if (sTestStruct.tag == 2) sTestStruct.data.permutation_2.tag
if (sTestStruct.tag == 2 && sTestStruct.data.permutation_2.tag == 0) sTestStruct.data.permutation_2.data.permutation_x
if (sTestStruct.tag == 2 && sTestStruct.data.permutation_2.tag == 1) sTestStruct.data.permutation_2.data.permutation_y
if (sTestStruct.tag == 3) sTestStruct.data.permutation_3.a
sTestStruct.b
((TransformedC) sTestStruct.c)
((TransformedD) sTestStruct.d).member_u
((TransformedD) sTestStruct.d).member_v
((TransformedDMemberW) ((TransformedD) sTestStruct.d).member_w).innermost
((TransformedD) sTestStruct.d).member_x
sTestStruct.e
((TransformedFinal) (TransformedFirst) sTestStruct.f)
sTestStruct.g
if (sTestStruct.g == 0) sTestStruct.h[0:5].permutation_0
```

Generated functions will look vaguely like this:

```c
void __generated(struct lu_BitstreamState* state) {
   __stream(state, sTestStruct.tag);
   if (sTestStruct.tag == 0) {
      __stream(sTestStruct.data.permutation_0.a);
      __stream(sTestStruct.data.permutation_0.b);
   } else if (sTestStruct.tag == 1) {
      __stream(sTestStruct.data.permutation_1.a);
   } else if (sTestStruct.tag == 2) {
      __stream(sTestStruct.data.permutation_2.tag);
      if (sTestStruct.data.permutation_2.tag == 0) {
         __stream(sTestStruct.data.permutation_2.data.permutation_x);
      } else if (sTestStruct.data.permutation_2.tag == 1) {
         __stream(sTestStruct.data.permutation_2.data.permutation_y);
      }
   } else if (sTestStruct.tag == 3) {
      __stream(sTestStruct.data.permutation_3.a);
   }
   __stream(sTestStruct.b);
   {
      struct TransformedC __transformed_0;
      DoTransformation(&sTestStruct.c, &__transformed_0);
      __stream(&__transformed_0);
   }
   {
      struct TransformedD __transformed_0;
      DoTransformation(&sTestStruct.d, &__transformed_0);
      __stream(__transformed_0.member_u);
      __stream(__transformed_0.member_v);
      {
         struct TransformedDMemberW __transformed_1;
         DoTransformation(&__transformed_0.member_w, &__transformed_1);
         __stream(__transformed_1.innermost);
      }
      __stream(__transformed_0.member_x);
   }
   __stream(sTestStruct.e);
   {
      struct TransformedFirst __transformed_0;
      struct TransformedFinal __transformed_1;
      DoTransform(&sTestStruct.f, &__transformed_0);
      DoTransform(&__transformed_0, &__transformed_1);
      __stream(&__transformed_1);
   }
   __stream(sTestStruct.g);
   if (sTestStruct.g == 0) {
      for(int i = 0; i < 5; ++i) {
         __stream(sTestStruct.h[i].permutation_0);
      }
   }
}
```

So we need to take the flat list of serialization items and generate a tree of blocks...

* A block (if/else-if) for each condition.
* A block for each transformation.
* A block (for-loop) for each array slice.

It would be easiest to convert the flat list of serialization items into a tree of blocks, and then generate code from the blocks, rather than trying to process a list and codegen a tree in real-time. But... what should go in the tree, and how should it be organized?

A function is a collection of instructions. The following instruction types exist:

* Instruction to serialize a **single** object.
  * This can include zero-padding used for unions, or we can give them a subclass -- whatever's easiest.
  * This can include omitted-and-defaulted values, or we can give them a subclass -- whatever's easiest.
* For-loop over an array **slice**.
* Block to **transform** a value.
* **Branch** on a condition.

The instruction types have the following elements:

* Single
  * Path to the to-be-serialized value.
* Slice
  * Start index
  * Count
  * Path to the array
  * List of instructions which are relative to the current array element
* Transform
  * Path to the to-be-transformed value
  * Type to transform to
    * This needs to be a list, for transitive types
  * List of instructions which are relative to the transformed value[^instruction-relative-to-temporary]
* Branch
  * Path to the value to be tested (the <dfn>condition operand</dfn>)
  * Integer constant to which the condition operand must be equal
  * List of instrutcions to execute if the condition is true
  * Branch to execute if the condition is false[^why-false-nested]

[^why-false-nested]: GCC parses if/else trees as ternary operators i.e. `cond_a ? body_a : (cond_b ? body_b : nothing)`. Ternaries (`COND_EXPR`) are allowed to have entire blocks (`BIND_EXPR`) as their true and false operands. We will therefore be generating branches the same way.

[^instruction-relative-to-temporary]: This means that it must be possible for all instruction types to refer to local variables in the function and its blocks, in addition to referring to file-scope `VAR_DECL`s and the array indices and `FIELD_DECL`s nested therein. We could use `decl_descriptor`s for function locals (and function parameters, where necessary), but this would require every instruction to be bifurcated (because the generated "read" and "save" functions will have different locals): we'd need `descriptor_pair` akin to `expr_pair` and `value_pair`. I think that may still be the cleanest approach.

I guess, then, that the above example would lead[^non-split-arrays] to these instructions:

* **[Single]** sTestStruct.tag
* **[Branch]** sTestStruct.tag == 0
  * *[If True]*
    * **[Single]** sTestStruct.data.permutation_0.a
    * **[Single]** sTestStruct.data.permutation_0.b
  * *[If False]*
    * **[Branch]** sTestStruct.tag == 1
      * *[If True]*
        * **[Single]** sTestStruct.data.permutation_1.a
      * *[If False]*
        * **[Branch]** sTestStruct.tag == 2
          * *[If True]*
            * **[Single]** sTestStruct.data.permutation_2.tag
            * **[Branch]** sTestStruct.data.permutation_2.tag == 0
              * *[If True]*
                * **[Single]** sTestStruct.data.permutation_2.data.permutation_x
              * *[If False]*
                * **[Branch]** sTestStruct.data.permutation_2.tag == 1
                  * *[If True]*
                    * **[Single]** sTestStruct.data.permutation_2.data.permutation_y
                  * *[If False]*
                    * *[none]*
          * *[If False]*
            * **[Branch]** sTestStruct.tag == 3
              * *[If True]*
                * **[Single]** sTestStruct.data.permutation_3.a
              * *[If False]*
                * *[none]*
* **[Single]** sTestStruct.b
* **[Transform]** sTestStruct.c
  * *[Transformed Type]* `TransformedC`
  * *[Transformed Variable]* `gw::decl::variable __read; gw::decl::variable __save;`
  * *[Instructions]*
    * **[Single]** &lt;transformed_c>
* **[Transform]** sTestStruct.d
  * *[Transformed Type]* `TransformedD`
  * *[Transformed Variable]* `gw::decl::variable __read; gw::decl::variable __save;`
  * *[Instructions]*
    * **[Single]** &lt;transformed_d>.member_u
    * **[Single]** &lt;transformed_d>.member_v
    * **[Transform]** &lt;transformed_d>.member_w
      * *[Transformed Type]* `TransformedDMemberW`
      * *[Transformed Variable]* `gw::decl::variable __read; gw::decl::variable __save;`
      * *[Instructions]*
        * **[Single]** &lt;transformed_d_w>.innermost
    * **[Single]** &lt;transformed_d>.member_x
* **[Single]** sTestStruct.e
* **[Transform]** sTestStruct.f
  * *[Transformed Type]* `TransformedFirst` -> `TransformedFinal`
  * *[Transformed Variable]*`gw::decl::variable __read; gw::decl::variable __save;`
  * *[Instructions]*
    * **[Single]** &lt;transformed_f>
* **[Single]** sTestStruct.g
* **[Branch]** sTestStruct.g == 0
  * *[If True]*
    * **[Slice]** sTestStruct.h
      * *[Start]* 0
      * *[Count]* 5
      * *[Instructions]*
        * **[Single]** sTestStruct.h[0:5].permutation_0
  * *[If False]*
    * *[none]*

[^non-split-arrays]: If an array fits entirely in a sector, then it won't be expanded even into a slice (i.e. we'll have `foo`, not `foo[0:10]`). We'll need to be on guard for that when we loop over serialization items, and make sure to convert these whole arrays into slices as well.

Once we've processed a flat list of serialization items into a tree of instructions, those instructions then convert pretty directly to generated code:

* Single
  * Read:  function calls[^whole-struct-functions] and assignment operators as appropriate
  * Write: function calls
* Slice
  * For-loop
* Transform:
  * A local block (`BIND_EXPR`)
  * One local variable per type we transform to/through
  * Statements applied to the last transformed variable go directly in the block
* Branch:
  * A conditional expression to test the condition value
  * A `COND_EXPR` ternary, with a `BIND_EXPR` (local block) as the "if true" operand

[^whole-struct-functions]: We want whole-struct functions to be shared across all sectors; yet we want to use the same code for generation a sector versus a whole struct function (i.e. in both cases we should be acting on a list of serialization items). The easiest way would be to just have a `whole_struct_function_dictionary` class which owns the functions, and allow different `function_generator`s to share it.




### Overall

I want to rewrite the codegen system, with the following goals:

* Apply non-struct `VAR_DECL`s as identifiers to serialize.
* Properly implement code generation for transforms.
  * This is harder than it seems because a transformed type may be a struct or array, requiring us to recursively handle it (potentially splitting it across sector boundaries) the same as anything else.
  * This is harder than it seems due to the need to handle transitive transforms, i.e. A to B to C such that when serializing an A, we use the A-to-B functions to make a B, use the B-to-C functions to make a C, and encode the C into the bitstream.
* Put us in a better position to support tagged unions in the future, and the branching they will require.

The old XML-based model entailed generating a list of "serialization items" representing every single field that we'd write into the serialized output, and then traversing over those and "expanding" them whenever an item didn't fit, such that we'd end up with one flat list of objects to serialize per sector and we'd then generate the code for those sectors. We need to go back to that model.

1. Loop over all top-level identifiers, which we should allow to be `VAR_DECL`s of any type.
2. Generate "serialization items" wrapping each.
3. Loop over all serialization items, checking against the sector size and bits remaining. If a serialization item doesn't fit, then expand it and try again &mdash; recursively expanding items until they fit.
4. Once we've got one flat list of serialization items per sector, generate the code from those.
  * We'll want to detect[^stems] when a group of consecutive serialization items have a common root, e.g. members of a transformed struct or the elements in a contiguous slice[^no-slices] of a containing array, and intelligently handle that case: we should transform once and serialize the transformed members, rather than repeatedly transforming once per member; we should use a for loop for array slices.

[^stems]: Serialization items are analogous to paths, so detecting whether two serialization items have a common stem is conceptually similar to detecting whether two paths have a common stem.

[^no-slices]: Encoding slice information into the serialization items is not viable.[^but-unions-need-slicing] If an array is right at the end of a sector, such that its first element won't fit, we would expand the array, recursively expand that first element as needed, and then we'd be left with all the other array elements expanded (i.e. `foo[1]`, `foo[2]`, `foo[3]`) in the next sector. No -- easiest to just only allow expanding serialization items, not trying to slice them, and then unify slices as a post-processing step.

### Serialization items

Serialization items are best conceptualized as being similar to `serialization_value_path` in the previous codebase, except that they also capture transforms. Examples include:

* `top_level_var_or_param`
* `top_level_var_or_param.member[2]`
* `((as transformed_type) transformed_container).member`
* `((as transformed_type_inner) ((as transformed_type_outer) transformed_container).transformed_member)`
* `((as transitively_transformed_type) (as transformed_type) transformed_container)`

Of course, serialization items do need to have *some* way of knowing what object they describe (and what its type and bitpacking options are), so that we know how to expand them. We can achieve this by having them refer to descriptor objects stored elsewhere. But... how should we rewrite and simplify those? See the "Descriptors" section for info.

#### Unions

We want to support tagged unions in the future. This... may actually be achievable. All we'd have to do is allow serialization items to optionally specify a condition (in the form of the serialization path to the tag field, and the value it must have). Then, we'd just preemptively expand unions. An example, with an externally tagged union:

```c
static struct Foo {
   int tag;
   LU_BP_UNION_TAG(tag) union {
      LU_BP_TAGGED_ID(0) int a;
      LU_BP_TAGGED_ID(1) int b;
      LU_BP_TAGGED_ID(2) int c;
   } bar;
} sFoo;
```

Assuming we have a single serialization item for `sFoo` as a whole, that item would expand to:

* `sFoo.tag`
* [`sFoo.tag` is 0] `sFoo.bar.a`
* [`sFoo.tag` is 1] `sFoo.bar.b`
* [`sFoo.tag` is 2] `sFoo.bar.c`

And if we have an internally tagged union?

```c
LU_BP_UNION_INTERNAL_TAG(tag)
static union Bar {
   LU_BP_TAGGED_ID(0) struct {
      int tag;
      int weather;
      int soil;
   } a;
   LU_BP_TAGGED_ID(1) struct {
      int tag;
      int climate;
      int humidity;
   } b;
} sBar;
```

Assuming we have a single serialization item for `sBar` as a whole, that item would expand to:

* `sBar.a.tag` (just use the first union member that has the common tag)
* [`sBar.a.tag` is 0] `sBar.a.weather`
* [`sBar.a.tag` is 0] `sBar.a.soil`
* [`sBar.a.tag` is 1] `sBar.b.climate`
* [`sBar.a.tag` is 1] `sBar.b.humidity`

The union members preemptively expand because we can't serialize them as a whole (since we've already read part of them: the embedded tag).

(We actually need to be able to store multiple AND-linked conditions, to account for nested unions. Generating the code for if/else trees would basically be similar to checking whether two paths have the same stem -- reusing an if/else branch for all consecutive items whose conditions have a common stem.)

### Descriptors

In the old codebase, we have `struct_descriptor` and `member_descriptor` which reflect the bitpacking options applied to struct types and their members, along with `member_descriptor_view` which is used to traverse these values during codegen. In the new codebase, we just need the following:

* `decl_descriptor`, to load and cache the bitpacking options that are in effect for any given `VAR_DECL`, `PARM_DECL`, or `FIELD_DECL`.
  * If the `PARM_DECL` is a pointer, then we assume we're dealing with a whole-struct function's argument and we act on the pointed-to type.

We'd store a map of `tree`s (i.e. tree node pointers) to `decl_descriptor` objects, and have [the path segments in] serialization items store non-owning pointers to the relevant `decl_descriptor`s.

The only purpose that `struct_descriptor` has in the old codebase is as a means of generating and reusing `member_descriptor`s, and for dealing with top-level struct values (since `member_descriptor` in the old codebase is for `FIELD_DECL`s only). When we generate whole-struct functions for a type, we do index them by `struct_descriptor` in the old codebase, but this is little better than indexing them by the raw `gw::type::base`.

In the new codebase, when we encounter a `DECL` whose value type is a struct (or an array thereof), we can just manually walk the struct's members each time and get-or-create `decl_descriptor`s for them. The only value a counterpart to `struct_descriptor` would offer is caching, for faster access to the `FIELD_DECL` descriptors. We could implement such a counterpart for that reason in the future, but for now, I want to keep things as minimalist as possible until I've got something working and until I've got a good mental model of it.

Serialization items could be thought of, then, as:

```c++
namespace codegen::serialization_items {
   class segment {
      public:
         struct array_access {
            size_t start = 0;
            size_t count = std::numeric_limits<size_t>::max();
         };
         
         const decl_descriptor* desc = nullptr;
         
         // When inspecting an array, e.g. foo[2][3].
         // When the accessor is foo[i], use a "slice" access with count = max.
         std::vector<array_access> array_accesses;
         
         // Listed in order applied, such that the type to be written to the bitstream is 
         // last in the list.
         std::vector<gw::type::base> transformations;
         
      public:
         const bitpacking::data_options::computed& options() const noexcept {
            return desc->options;
         }
         
         size_t size_in_bits() const noexcept {
            auto  size    = desc->innermost_size_in_bits();
            auto& extents = desc->array.extents;
            for(size_t i = this->array_accesses.size(); i < extents.size(); ++i) {
               size *= extents[i];
            }
            if (!this->array_accesses.empty()) {
               auto&  access = this->array_accesses.back();
               size_t count  = access.count;
               if (access.count == std::numeric_limits<size_t>::max()) {
                  size_t extent = extents[this->array_accesses.size() - 1];
                  count = extent - access.start;
               }
               size *= count;
            }
            return size;
         }
   };
   
   // Called a "basic" path because we'll have to deal with tagged unions in the future too.
   class basic {
      public:
         std::vector<segment> segments;
         
         const decl_descriptor& descriptor() const noexcept {
            return *segments.back().desc;
         }
         const bitpacking::data_options::computed& options() const noexcept {
            return *segments.back().options();
         }
         size_t size_in_bits() const noexcept {
            return segments.back().size_in_bits();
         }
         
         // Depends on the type of the referred-to DECL.
         bool can_expand() const;
         
         // Behavior varies depending on whether the type of the referred-to DECL 
         // is a struct (expand to its members) or an array (expand to its elements).
         std::vector<basic> expand() const;
   };
}
```

The root segment of a serialization item would refer either to a `VAR_DECL` or a `PARM_DECL`. The latter would be the case when dealing with whole-struct functions.


## `lu-bitpack`

### Short-term

* Finish replacing `struct_descriptor`, `member_descriptor`, and `member_descriptor_view` with `type_descriptor`, `data_descriptor`, and `data_descriptor_view`. The main goal of this change is to be able to support `VAR_DECL`s as top-level to-be-serialized identifiers: `data_descriptor` can wrap a `VAR_DECL` or a `FIELD_DECL`, while also capturing bitpacking options for structs so we can record them in the XML output.
  * Maybe it'd be easier to just find-and-replace references to the old types en masse, and then fix compiler errors one by one afterward.
  * `type_descriptor` and `data_descriptor` still need to properly compute the size in bits of the transformed type, when transform options are used.
* Pre-pack/post-unpack functions
  * Steps needed to allow the use of transforms on top-level structs
    * We need to store bitpacking options on `codegen::struct_descriptor`
    * XML needs to serialize transform options on struct descriptors
    * `serialization_value::is_transformed` needs to handle top-level structs
    * `sector_functions_generator::_serialize_transformed` needs to handle `serialization_value`s that are top-level structs
  * When dealing with a transformed object, `codegen::struct_descriptor::size_in_bits` and `codegen::member_descriptor::size_in_bits` need to return the serialized size in bits of the transformed type.
  * Codegen:
    * In order to handle splitting a transformed struct type across sector boundaries, we need to add a branch to `sector_functions_generator::_serialize_value_to_sector` just before the `object.is_struct()` case, wherein we check if `object.is_transformed()` *and* if the transformed type is a struct. If so, we'd need to...
      * Get/create a descriptor for the transformed type.
      * Generate a `gw::expr::local_block`.
      * Define a variable of the transformed type.
      * Generate a call to the pre-pack function.
      * Wrap the transformed-type variable in a `serialization_value`, and then recurse on that value's struct members.
        * Use `value.transform_to(transformed_type_name).to_member(m)` as the serialization path when recursing on each member.
    * We likewise need to handle the case of the transformed type being an array or primitive. Primitives are easy, but I'm unsure how to handle the case of transformation to an array.
      * For arrays, it might be easiest to generate a dummy `member_descriptor`, wrap it in a view, and then recurse similarly to how we would for structs.
  * At some point, we should look into detecting circular transforms at attribute-handling time, as I believe they'd cause us to spin in an infinite loop within codegen if encountered.
* Implement statistics tracking (see section below).
* I don't think our `std::hash` specialization for wrapped tree nodes is reliable. Is there anything else we can use?
  * Every `DECL` has a unique `DECL_UID(node)`, but I don't know that these would be unique among other node types' potential ID schemes as well.
  * `TYPE_UID(node)` also exists.
  * Our use case is storing `gw::type::base`s inside of an `unordered_map`. We could use GCC's dedicated "type map" class, or &mdash; and this may be more reliable &mdash; we could use a `vector` of `pair`s, with a lookup function which looks for an exact type match or a match with a transitive typedef.

* The XML output has no way of knowing or reporting what bitpacking options are applied to integral types (as opposed to fields *of* those types). The final used bitcounts (and other associated options) should still be emitted per field, but this still reflects a potential loss of information. Can we fix this?
* It'd be very nice if we could find a way to split buffers and strings across sector boundaries, for optimal space usage.
  * Buffers are basically just `void*` blobs that we `memcpy`. When `sector_functions_generator::_serialize_value_to_sector` reaches the "Handle indivisible values" case (preferably just above the code comment), we can check if the primitive to be serialized is a buffer and if so, manually split it via similar logic to arrays (start and length).
  * Strings require a little bit more work because we have to account for the null terminator and zero-filling: a string like "FOO" in a seven-character buffer should always load as "FOO\0\0\0\0"; we should never leave the tail bytes uninitialized. In order to split a string, we'd have to read the string as fragments (same logic as splitting an array) and then call a fix-up function after reading the string (where the fix-up function finds the first '\0', if there is one, and zero-fills the rest of the buffer; and if the string requires a null-terminator, then the fix-up function would write that as well). So basically, we'd need to have bitstream "string cap" functions for always-terminated versus optionally-terminated strings.
* Long-term plans below
  * Pre-pack and post-unpack functions
  * Union support

### Short-term advanced

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


### Long-term

#### Pre-pack and post-unpack functions

**BLUF:** Bitpacking involves finding the most compact possible representation of a given struct. However, we may wish to transform certain structs just before they're saved or just after they're read, so that they can be compacted further. We should offer a way to specify functions that can be called automatically to perform these operations. We currently allow the user to specify the names of functions to transform a data member, but we don't allow them to specify this on a type definition (we simply don't notice it there), nor have we actually implemented code to invoke those functions.

My use case involves some kinds of data that ought to be packed in a completely separate format from the way it's stored in memory.

Imagine a video game inventory that is subdivided into different categories &mdash; not just in terms of how we present data to the user, but in terms of the underlying in-memory representation as well &mdash; in an item system where every item has a globally unique ID. For example, you may have a "weapons" category and an "armor" category, with items like these:

| Global ID | Item |
| -: | :- |
| 0 | None |
| 1 | Wooden Sword |
| 2 | Stone Sword |
| 3 | Iron Sword |
| 4 | Gold Sword |
| 5 | Diamond Sword |
| 6 | Iron Armor |
| 7 | Gold Armor |
| 8 | Diamond Armor |

The highest possible global item ID is 8, requiring four bits to encode. If you can carry up to 10 weapons and up to 5 armors, then this means we need 4 &times; 15 = 60 bits for the entire inventory. Recall, however, that the inventory is categorized under the hood as well. Why not take advantage of that for serialization?

It will generally be easiest to use global item IDs in memory, while the game is running; but for the sake of serialization, we can give each item category its own "local" ID space:

| Global ID | Local ID | Item |
| -: | -: | :- |
| 0 | 0 | None |
| 1 | 1 | Wooden Sword |
| 2 | 2 | Stone Sword |
| 3 | 3 | Iron Sword |
| 4 | 4 | Gold Sword |
| 5 | 5 | Diamond Sword |
| 6 | 1 | Iron Armor |
| 7 | 2 | Gold Armor |
| 8 | 3 | Diamond Armor |

The highest local weapon ID is 5, requiring only 3 bits to encode; and the highest local armor ID is 3, requiring only 2 bits to encode; and so now, the total serialized size is (3 * 10) + (2 * 5) = 40 bits. The thing is, this requires being able to transform the in-memory data before it's bitpacked, and transform the bitpacked data after it's read but before it's stored.

Taking advantage of this idea using a future version of this plug-in might look like this:

```c
#define LU_BP_MAX(n) __attribute__((lu_bitpack_range(0, n)))
#define LU_BP_TRANSFORM(pre, post) __attribute__((lu_bitpack_funcs("pre_pack=" #pre ",post_unpack=" #post )))

struct PackedInventory {
   LU_BP_MAX(5) uint8_t weapons[10];
   LU_BP_MAX(3) uint8_t armors[5];
};

void MapInventoryForSave(const struct Inventory* in_memory, struct PackedInventory* packed);
void MapInventoryForRead(struct Inventory* in_memory, const struct PackedInventory* packed);

LU_BP_TRANSFORM(MapInventoryForSave, MapInventoryForRead)
struct Inventory {
   uint8_t weapons[10];
   uint8_t armors[5];
};
```

(Yes, the example above could be accomplished using the `lu_bitpack_range(min, max)` attribute, but only because I made the item categories contiguous for clarity. In a real-world scenario, global item IDs may not be sorted by category: weapons and armor may be interleaved together, and in that case, you'd need a more complex mapping between global and category-local IDs.

(Additionally, it's valuable to have the generated serialization code handle data transformations because, for example, `Inventory` may be a member of some far larger struct that we want to save, rather than a freestanding struct that we can manually convert before and after initiating the save and load operations.)

##### Details

We need to be able to change the type of the value we're packing. This means that we have to generate code like the following for save:

```c
// local block to limit the scope of `__v`
{
   // generated variable
   struct PackedInventory __v;
   
   // extra call
   MapInventoryForSave(&currentStruct->inventory, &__v);
   
   __lu_bitpack_write_PackedInventory(state, &__v);
}
```

And like the following for read:

```c
{
   // generated variable
   struct PackedInventory __v;
   
   __lu_bitpack_read_PackedInventory(state, &__v);
   
   // extra call
   MapInventoryForRead(&currentStruct->inventory, &__v);
}
```

This in turn means that we need to infer the type of `__v` from the types used in the bitpack functions. That is:

* Let the *in situ* type of the to-be-packed member be *T*.
* Let the *transformed* type of the to-be-packed member be *U*. This is the type that will be used for the generated variable `__v` above.
* The pre-pack function must be of the form `void a(const T*, U*)`.
* The post-unpack function must be of the form `void b(T*, const U*)`.

The actual logistics of this include:

* When we generate computed bitpacking options for a to-be-transformed member, we need to base those (and the `member_kind`) on the transformed type, not the in situ type. We will of course still need to be aware of the original type, and of the fact of there being a transformation.
  * But wait. We currently validate a lot of options when we see the attributes for them, such as erroring if we see integer options on a non-integral type or field. That... isn't going to play so well with transforms.
    * Maybe we should just flat-out make it so that transforms can't be used alongside other options: if you use a transform on a type or field, then you have to specify all further bitpacking options on the transformed type, not on the in situ type or field.

Other support requirements:

* We should rename `lu_bitpack_funcs` to `lu_bitpack_transform`.
* When using transformation functions, the `member_kind` of a to-be-serialized member should depend on the transformed type, not the in situ type.
  * ...*but* we should still enforce per-type limits based on the data member's original type. That is: a struct-type data member is forbidden from specifying integer-type bitpacking options *even if* it uses transformation functions to be serialized as an integer. Those options should instead be specified on the integer type (and if it's a built-in e.g. `uint8_t`, then use a typedef and put the options on that typedef; I've already implemented error messaging in the `lu_bitpack_range` attribute handler instructing users to do this).


#### Union support

**BLUF:** Currently, we don't support unions at all. We could add preliminary support for them by allowing the user to mark them as opaque buffers, but we want to support bitpacking them in the future. Bitpacking unions poses a challenge primarily due to the need to split unions across sector boundaries for the most robust packing possible.

The current approach to bitpacking is always-forward: we generate one per-sector function at a time and track the amount of bits currently remaining in that sector. In order to properly split unions, however, we'll want to instead preemptively "open" every per-sector function at the start of generation, and then emit code into the "current" function. When we encounter a union and we can tell that it won't fit in the current sector, we'll...

1. Create an if/else tree branching on the union's tag, in both the current sector and the next sector. We want one branch per serializable union member.
2. For each union member *n*...
   1. Generate as much code into the *n* branch in the current sector.
   2. Emit the rest of the code into the *n* branch in the next sector.
   3. If *n* is not the largest member in the union, then pad with zeroes until the bitpacked sizes match.
3. Once all union members are done, advance code generation as a whole to the next sector.

This approach can generalize to unions that need to split multiple times, because they are large enough (and positioned close enough to the end of one sector) to span across three or more sectors.

Of course, unions won't only contain primitive members; they may also contain nested structs, arrays, and even other unions. This means that we'd have to modify the entire overall algorithm, conceptualizing ourselves as always emitting code into one branch among potentially several, and tracking bitcounts for the current branch; and that branch must itself be able to recursively split. Potentially that means branching the entire serialization state:

```c++
struct sector_state {
   std::vector<in_progress_function_pair> sector_functions;
};

// State for any given call stack frame in the recursive algorithm.
struct branched_sector_state {
   sector_state& common;
   std::vector<gw::expr::local_block> branches;
   size_t sector_id;
   size_t bits_remaining;
};

//
// When we encounter a union, we loop over its members and preemptively create 
// gw::expr::ternary instances, to generate the needed if/else trees in the 
// current and next sectors. We store these in a vector such that if we're at 
// sector A and about to split, then `blocks[x][A + y]` is the `local_block` 
// for the Xth union member's Yth branch.
//
// Then, we loop over each union member again. For each union member, we create 
// a copy of our current `branched_sector_state`. We set the copy's `branches` 
// list to `blocks[x]` and run it.
//
// Finally, after all union members are done, we take our `branched_sector_state` 
// and advance it to the end of the union.
//
```

We want to support two kinds of unions: *externally tagged unions* and *internally tagged unions*.

##### Externally tagged unions

An externally tagged union relies on a previous-sibling member to indicate the union's active member, as in the following example, where `Foo::tag` is the tag for `Foo::data`. We can support externally tagged unions by requiring that the union itself be annotated to indicate a previous-sibling member to serve as a tag, and requiring that each member of the union be annotated with its corresponding tag value. Notably, this would allow us to support situations where the tag values don't match the order of the union's members.

```c
#define LU_BP_TAG(name)    __attribute__((lu_bitpack_union_tag(name)))
#define LU_BP_TAG_VALUE(n) __attribute__((lu_bitpack_union_tagged_id(n)))

struct Foo {
   int tag = 2;
   LU_BP_TAG(tag) union {
      LU_BP_TAG_VALUE(0) struct {
         // ...
      } member_0;
      LU_BP_TAG_VALUE(1) struct {
         // ...
      } member_1;
      LU_BP_TAG_VALUE(2) struct {
         // ...
      } member_2;
   } data;
};
```

Requirements that we'll need to validate:

* Marking a freestanding union type as externally tagged is an error. However, it's fine to mark a freestanding union type's members as having tag values, and then mark individual data members of that type which appear inside of other structs.
* The union's tag member must be an integer type.
* The union's tag member must not be marked as omitted from bitpacking.
* The union's tag member must be a previous-sibling of the union. Probably the easiest way to verify this would be by comparing `offsetof`s; iterating over the containing struct's members isn't as likely to work due to the risk of missing anonymous structs.
* Each member of the union must be given a tag value, or must be marked as omitted from bitpacking. If any union member lacks a tag value and isn't marked as being omitted from bitpacking, then that's an error.
  * No two members of the union can have the same tag value.
  * All tag values assigned to members of the union must be representable within bitpacking. If we pack the tag such that the values [2, 7] can be serialized, then no union member may have a tag value like 1 or 8.

##### Internally tagged unions

An internally tagged union only contains structs as its members. All union members have the first *n* fields in common where *n* is at least 1, and one of these shared fields is the union tag.

```c
#define LU_BP_TAG_INTERNAL(name) __attribute__((lu_bitpack_union_internal_tag(name)))
#define LU_BP_TAG_VALUE(n)       __attribute__((lu_bitpack_union_tagged_id(n)))

LU_BP_TAG_INTERNAL(kind) union Foo {
   LU_BP_TAG_VALUE(0) struct {
      int   kind;
      int   divergent_0;
   } member_0;
   LU_BP_TAG_VALUE(1) struct {
      int   kind;
      float divergent_1;
   } member_1;
};
```

Requirements:

* Freestanding union types may be marked as internally tagged (if all other requirements are met) and, if so marked, may be used as top-level data values to serialize.
* All of the union's members must be of `struct` types.
* All of the union's members must have their own first *n* members be identical (same names, types, and if present, initializers) given some value of *n* > 0.
  * We may decide to only consider members that are not marked as omitted from bitpacking, such that omitted members may be interleaved between them.
* The identifier given as the union tag must be one of those *n* nested members.
  * That member must be an integer type.
  * That member must not be marked as omitted from bitpacking.
* Each member of the union must be given a tag value, or must be marked as omitted from bitpacking. If any union member lacks a tag value and isn't marked as being omitted from bitpacking, then that's an error.
  * No two members of the union can have the same tag value.
  * All tag values assigned to members of the union must be representable within bitpacking. If we pack the tag such that the values [2, 7] can be serialized, then no union member may have a tag value like 1 or 8.
