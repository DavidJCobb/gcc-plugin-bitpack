
# `gcc_wrappers`

An attempt at creating a more type-safe interface to GCC's untyped `tree` nodes.

Goals:

* Stricter typing
* Add convenient, relevant accessors, so I don't constantly have to rummage around GCC internals
  * There is no immediately obvious correlation between the name of the header that declares a function and the name of the file that defines it, and most search engines I've been using -- even those specifically built for code -- miss identifiers *constantly*, so this is genuinely quite painful.
* Improved ergonomics

## To-do list

### Better interoperability of wrappers

I should make it possible to do things like the following:

```c++
namespace gw {
   using namespace gcc_wrappers;
}

// Assume:
//    state_type == instance of gw::type
//    loop_index == instance of gw::decl::variable

gw::statement_list statements;

// Declare a variable `__lu_bitstream_state` of type `state_type`
auto state_decl = gw::variable::create("__lu_bitstream_state", state_type);
state_decl.mark_artificial();
state_decl.mark_used();

// Get access to static variable sStructA
gw::decl::variable object_decl;
object_decl.set_from_untyped(lookup_name(get_identifier("sStructA")));

// sStructA.a[i] = lu_BitstreamRead_u8(&__lu_bitstream_state, 6);
statements.append(
   gw::expr::assign(
      // sStructA.a[i]
      object_decl.access_member("a").access_array_element(loop_index),
      
      // lu_BitstreamRead_u8(&__lu_bitstream_state, 6);
      gw::expr::call(
         func_decl_to_call,
         state_decl.address_of(), // returns gw::expr::addressof
         gw::integer_constant(uint8_type_node, 6)
      )
   )
);
```

This requires designing a common interface: anything that an `ARRAY_REF` node can point to should offer the `access_array_element` accessor, for example.

#### Research

`COMPONENT_REF` (member access) can access members of the following, provided the underlying type (`TREE_TYPE`) of the thing being accessed is a `RECORD_TYPE` or `UNION_TYPE`:

* `VAR_DECL`
* Any `*_EXPR` whose result is of a `RECORD_TYPE` or `UNION_TYPE`
* Anything else?
  * `PARM_DECL`
  * `RESULT_DECL`?

`ARRAY_REF` can access array elements of...

* `VAR_DECL`
* Any `*_EXPR` whose result is of an `ARRAY_TYPE`
* Anything else?
  * `PARM_DECL`?
  * `*_REF`?

`ADDR_EXPR` (addressof, i.e. `&foo`) can be used on...

* `VAR_DECL`
* Anything else?
  * `PARM_DECL`?
  * `*_REF`?
  * Any `*_EXPR` that represents an addressable object?

`INDIRECT_REF` (pointer deference, i.e. `foo->`) can be used on...

* ?

There's no way to have a statically type-safe approach, because the result type can be transitive (e.g. we could get an `INTEGER_TYPE` from a `PREDECREMENT_EXPR` that wraps an `ARRAY_REF` that wraps a `COMPONENT_REF`). We'll just have to assert type-correctness at run-time. Thankfully, all `*_EXPR` nodes declare their result type, so we don't have to traverse through whole expression trees to do type chcking.


### Divide the `type` wrapper up the same way as the `decl` and `expr` wrappers

Wrappers around `RECORD_TYPE` and `UNION_TYPE` should offer convenient access to the types' members, handling for the user the work of separating data and function members, and separating static and non-static members.

* `FIELD_DECL`: non-static data member
* `VAR_DECL`: static data member
* `CONST_DECL`: ?
* `TYPE_DECL`: type declaration or typedef

Wrappers around `FUNCTION_TYPE` should offer easy access to the return type (`TREE_TYPE(fn_type)`), the argument types and default value expressions (`TYPE_ARG_TYPES(fn_type)`), and an easy way to check if the function is varargs.[^function-type] Wrappers around `METHOD_TYPE` are similar.[^method-type]

[^function-type]: From the GCC documentation: "The `TREE_TYPE` gives the return type of the function. The `TYPE_ARG_TYPES` are a `TREE_LIST` of the argument types. The `TREE_VALUE` of each node in this list is the type of the corresponding argument; the `TREE_PURPOSE` is an expression for the default argument value, if any. If the last node in the list is `void_list_node` (a `TREE_LIST` node whose `TREE_VALUE` is the `void_type_node`), then functions of this type do not take variable arguments. Otherwise, they do take a variable number of arguments. Note that in C (but not in C++) a function declared like `void f()` is an unprototyped function taking a variable number of arguments; the `TYPE_ARG_TYPES` of such a function will be `NULL`."

[^method-type]: `METHOD_TYPE` types store `decltype(*this)` as `TYPE_METHOD_BASETYPE(fn_type)`. Additionally, the `TYPE_ARG_TYPES` includes the `this` pointer.

Wrappers around `ENUMERAL_TYPE` should offer easy access to the enum members[^walk-enum-members] and information on the enum's underlying type[^enum-underlying].

[^walk-enum-members]: You can walk an enum's members via the `TREE_LIST`-type node `TYPE_VALUES(type)`. For each list item, `std::string_view(IDENTIFIER_POINTER(TREE_PURPOSE(item)))` is the member name and `TREE_VALUE(item)` is an `INTEGER_CST`-type node holding the member value. (If needed, you can also backtrack from the `CONST_DECL` enum members to the enum type via `TREE_TYPE(member)`.) Additionally, `TYPE_MIN_VALUE(type)` and `TYPE_MAX_VALUE(type)` each return `INTEGER_CST`-type nodes indicating the lowest and highest values among the enum type's members.

[^enum-underlying]: `TYPE_PRECISION(type)` stores the number of bits used to represent an enum type's values. If there are no negative members in the enum, then `TYPE_UNSIGNED(type)` will be truthy (even if the enum was declared with a signed underlying type?).

Wrappers around `INTEGER_TYPE` should offer information about the integer type, e.g. the number of defined bits in the type (`(unsigned int)TYPE_PRECISION(type)`), whether the type is signed (`TYPE_UNSIGNED(type)`), and the lowest and highest representable values (`TYPE_MIN_VALUE(type)` and `TYPE_MAX_VALUE(type)`, both offering `INTEGER_CST`-type nodes).

Wrappers around `REAL_TYPE` should offer the number of defined bits in the type (`(unsigned int)TYPE_PRECISION(type)`).

Wrappers around `FIXED_POINT_TYPE` should offer the number of defined bits in the type (`(unsigned int)TYPE_PRECISION(type)`), the number of fractional and integral bits (`TYPE_FBIT(type)` and `TYPE_IBIT(type)`), whether the type is signed (`TYPE_UNSIGNED(type)`), and whether the type is saturating (`TYPE_SATURATING(type)`).

Wrappers around `ARRAY_TYPE` should offer easy access to the array extent.

In general, any transformation available in C++'s `<type_traits>` library should be replicated on all applicable types.

### Type wrappers

#### Base

Offer easy access to:

* The type alignment in bits (`/*int*/ TYPE_ALIGN(type)`)
* The type size in bits (`TYPE_SIZE(type)` is an `INTEGER_CST`-type node or, if the type is incomplete, `NULL_TREE`)
* The type declaration (`TYPE_NAME(type)` is a `TYPE_DECL`, not an `IDENTIFIER_NODE`)
* `TYPE_CANONICAL(type)`
* `TYPE_MAIN_VARIANT(type)`

Figure out a reliable way of equality-comparing types that doesn't rely on `same_type_p` or `comptypes`, since we can't link to those (AFAIK?) when we're invoked by the C compiler (as opposed to the C++ compiler; the plug-in headers do at least include the C++ versions of those functions).


### Decl wrappers

#### Base

Offer easy access to:

* `std::string_view(IDENTIFIER_POINTER(DECL_NAME(decl)))`: declaration name
* `std::string_view(DECL_SOURCE_FILE(decl))`
* `DECL_SOURCE_LINE(decl)`, an `int`
* read access to `DECL_ARTIFICIAL(decl) == 1`
* `DECL_CONTEXT(decl)`
  * But what kinds of nodes can this be?

#### Function decl

Offer easy access to:

#### Type decl (`TYPE_DECL`)

Offer easy access to:

* `TREE_TYPE(type_decl)`: the declared type (a `*_TYPE` node)
* `DECL_ORIGINAL_TYPE(type_decl)`: If `type_decl` is a typedef, then this is the type node (`*_TYPE`) that `type_decl` is a new name for.


### Expr wrappers

#### Base

#### To-be-implemented types

`INTEGER_CST` represents an integer constant. They are not always `int`; the underlying type can be accessed via `TREE_TYPE(node)`. The value is `((TREE_INT_CST_HIGH(node) << HOST_BITS_PER_WIDE_INT) + TREE_INST_CST_LOW(node))` i.e. the value may be wider than whatever a "wide int" is on the platform the compiler and plug-in are compiled on. (`HOST_BITS_PER_WIDE_INT` is always at least 32.)

* Use `TREE_INT_CST_LOW` to get the low wide-int of the value i.e. the low 32 bits.
* Use `tree_int_cst_lt` and `tree_int_cst_equal` to compare two full integer constants. The full value is compared and both types are assumed to have the same signedness; if you want integer promotion, etc., you need to do it yourself.
* Use `tree_int_cst_sgn` to get a constant's sign (-1, 0, or 1). This takes the type into account, i.e. unsigned-type constants are never negative.

Other expression types include...

* `BIND_EXPR`: A local block.
  * Operand 0 is a `TREE_LIST` of local variables (`VAR_DECL`).
  * Operand 1 is the body of the block.
* `CALL_EXPR`: Function calls.
  * Operand 0 is a `POINTER_TYPE` expression indicating the function to call.
  * Operand 1 is a `TREE_LIST` full of the arguments to pass, from left to right.
    * Each item's `TREE_VALUE` is an expression producing the argument to pass. The `TREE_PURPOSE` is unspecified/undefined.
    * For calls to non-static member functions, `this` is an argument and is passed here.
    * Default argument values are included here even if the original source code didn't explicitly specify those arguments.
* `EXIT_EXPR`: Conditionally breaks out of the nearest containing `LOOP_EXPR`. The sole operand is the condition to test, and we exit if the result is nonzero.
  * `EXIT_EXPR` should only appear somewhere inside of a `LOOP_EXPR`.
* `LOOP_EXPR`: An infinite loop. `LOOP_EXPR_BODY(expr)` is the loop body. The loop executes repeatedly until an `EXIT_EXPR` is run.
* `MODIFY_EXPR`: Assignment.
  * Operand 0 is the LHS and is "a `VAR_DECL`, `INDIRECT_REF`, `COMPONENT_REF`, or other lvalue."
  * Operand 1 is the RHS.
  * These also handle compound assignment, i.e. `i += 2` parses to `i = (i + 2)`.
* `INIT_EXPR`: Same as `MODIFY_EXPR`, but used for when a variable is initialized.
* `SAVE_EXPR`: Wraps an expression (the first operand) and prevents the expression from being executed more than once (e.g. memoization). The purpose is to avoid repetition of an expression's side-effects where the expression's result should be used more than once.
* `STMT_EXPR`: GCC extension for treating statements as expressions e.g. `return ({ int j = 3; j + 1; });`.
  * `STMT_EXPR_STMT(expr)` is the statement contained in the expression, and is always a `COMPOUND_STMT`.
  * If the last direct child of `STMT_EXPR_STMT(expr)` is an `EXPR_STMT`, then the value it computes is the result of the `STMT_EXPR`. Otherwise, the `STMT_EXPR` yields no value and its type is `void`.

Unusual ones (see [docs](https://gcc.gnu.org/onlinedocs/gcc-3.4.3/gccint/Expression-trees.html#Expression-trees)):

* `AGGR_INIT_EXPR`
* `CLEANUP_POINT_EXPR`
* `CONSTRUCTOR`
* `COMPOUND_LITERAL_EXPR`
* `TARGET_EXPR`
* `VA_ARG_EXPR`
* `VTABLE_REF`

Arithmetic operators:

* `ABS_EXPR`: unary absolute value of an integer or real (check the expression node's type to know which).
* `PLUS_EXPR`, `MINUS_EXPR`, and `MULT_EXPR`: Addition, subtraction, and multiplication, where the first operand is LHS and the second is RHS. The operands may be integral or real types, but you'll never see an integral paired with a real or vice versa here.
* `TRUNC_DIV_EXPR`: Integer division, rounding toward zero.
* `TRUNC_MOD_EXPR`: `a - (a/b)*b` using integer division i.e. `TRUNC_DIV_EXPR`
* `RDIV_EXPR`: Floating-point division.
* `EXACT_DIV_EXPR`: Undocumented.
* `NEGATE_EXPR`: unary negation of an integer or real (check the expression node's type to know which).
* `PREDECREMENT_EXPR`: `--x` producing an integral, real, or bool.
* `PREINCREMENT_EXPR`: `++x` producing an integral, real, or bool.
* `POSTDECREMENT_EXPR`: `x--` producing an integral, real, or bool.
* `POSTINCREMENT_EXPR`: `x++` producing an integral, real, or bool.

Access operators:

* `ADDR_EXPR`: address-of operator used on an object or label.
  * Taking the address of a label is a GCC extension, and the resulting type is `void*`.
  * If the target isn't an lvalue, a temporary is created, and the address of the temporary is used.
* `ARRAY_REF`: array access; operands are the array and the index. The expression type must be the array's value type.
* `ARRAY_RANGE_REF`: array slice access; operands are the array and the start index. The expression type must be an array with the same value type, whose length is the length of the slice to be accessed.
* `COMPONENT_REF`: member access.
  * Operand 0 is the object to access a field of; `TYPE_CODE(TREE_TYPE(operand))` must be `RECORD_TYPE` or `UNION_TYPE`.
  * Operand 1 is the `FIELD_DECL` to access.
  * There already exists a `build_component_ref` function to build these accesses for you given an object, field identifier, and (for error reporting) source locations. However, its signature has changed occasionally from version to version of GCC.
* `INDIRECT_REF`: dereferencing of a pointer or reference.
* `NON_LVALUE_EXPR`: wraps a single operand to indicate that it's not an lvalue. Often, you can just unwrap it and deal with the operand directly.

Conversion and decomposition operators:

* `COMPLEX_EXPR`: construction of a `COMPLEX_TYPE`-type value given two operands: the real and imaginary parts. Both operands are of the same type, and can be integers or reals.
* `CONJ_EXPR`: "These nodes represent the conjugate of their operand."
* `CONVERT_EXPR`: conversion between two types that *may* require code generation. Never used for C++-specific conversions (e.g. between subclass and superclass pointers). Never used for user-defined conversions (the calls are made explicitly).
* `IMAGPART_EXPR` and `REALPART_EXPR`: Access the real or imaginary halves of a complex number (the sole operand).
* `FIX_TRUNC_EXPR`: conversion of a float to an integer or bool, truncating. The operand is always a real; the result, always an integer or bool.
* `FLOAT_EXPR`: conversion of an integer or bool to a float. The operand is always an integer or bool; the result, always a real.
* `NOP_EXPR`: conversion between two types that don't require any actual bytecode (e.g. from `char*` to `int*`, or from a pointer to a reference).

Comparison operators:

* `LT_EXPR`: Operands are either both integral or both real. Result is always integral or bool.
* `LE_EXPR`
* `GT_EXPR`
* `GE_EXPR`
* `EQ_EXPR`
* `NE_EXPR`

Logical operators:

* `TRUTH_NOT_EXPR`: logical-NOT of an integer or bool.
* `TRUTH_ANDIF_EXPR` and `TRUTH_ORIF_EXPR`: logical-AND and logical-OR. These operators short-circuit. Both operands must be integer or bools.
* `TRUTH_AND_EXPR`, `TRUTH_OR_EXPR`, and `TRUTH_XOR_EXPR`: non-short-circuiting logical AND, OR, and XOR operations. C and C++ don't have operators for these, but frontends can generate them if they can tell that short-circuiting doesn't matter.
* `THROW_EXPR`: a `throw` expression. The sole operand is all code needed to perform the operation *except* for the actual call to the `__throw` function. Generated by the compiler-internal function `emit_throw`.
* `COMPOUND_EXPR`: comma operator. First operand is computed and discarded; second operand is the result.
* `COND_EXPR`: ternary (`a ? b : c`).
  * `a` must be of boolean or integral type.
  * `b` and `c` must be the same type as the entire expression, *unless* they unconditionally throw or call a `noreturn` function, in which case their type should be `void`. This allows things like `(i >= 0 && i < 10) ? i : abort()` for array bounds checking to be encoded more-or-less directly into a `COND_EXPR`.
  * There's a GNU extension wherein `x ? : 3` is equivalent to `x ? x : 3`. The resulting `COND_EXPR` tree does not omit the second operand; it may be the same as the first operand (same tree node?) or it may use a `SAVE_EXPR` to avoid side-effects.

Bitwise operators:

* `BIT_NOT_EXPR`: bitwise-NOT of a single value, producing an integer.
* `LSHIFT_EXPR`: bitshift left. First operand is the integer to shift; second is any expression yielding the number of bits by which to shift. Result is undefined if the second expression is larger than the number of bits in the first operand's type (i.e. `TYPE_SIZE(TREE_TYPE(operand))`).
* `RSHIFT_EXPR`: arithmetic shift right (i.e. if the to-be-shifted integer is of a signed type, extend the sign bit). First operand is the integer to shift; second is any expression yielding the number of bits by which to shift. Result is undefined if the second expression is larger than the number of bits in the first operand's type (i.e. `TYPE_SIZE(TREE_TYPE(operand))`).
* `BIT_IOR_EXPR, `BIT_XOR_EXPR`, and `BIT_AND_EXPR`: bitwise inclusive-OR, exclusive-OR, and bitwise-AND. Both operands are integers.

Overall table:

| `TREE_CODE` | Operand 0 type | Operand 1 | Result type | Details |
| :- | :- | :- | :- | :- |
| `ABS_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | | `INTEGER_TYPE` or `REAL_TYPE` | Unary absolute value. |
| `ADDR_EXPR` | lvalue, temporary, or `LABEL_DECL` | | `POINTER_TYPE` | `&label` produces `void*` |
| `AGGR_INIT_EXPR` | ? | ? | ? | ? |
| `ARRAY_REF` | `ARRAY_TYPE` | | `TREE_TYPE(a)` | |
| `ARRAY_RANGE_REF` | `ARRAY_TYPE` | | `ARRAY_TYPE` | Result has the same value type as operand 0, copying and returning a slice of the array. |
| `ASM_EXPR` | ? | ? | ? | Inline assembly. |
| `BIND_EXPR` | `TREE_LIST` of `VAR_DECL` | block body | ? | Local block of statements. |
| `BIT_AND_EXPR` | `INTEGER_TYPE` | `INTEGER_TYPE` | `INTEGER_TYPE` | Bitwise-AND (`&`). |
| `BIT_IOR_EXPR` | `INTEGER_TYPE` | `INTEGER_TYPE` | `INTEGER_TYPE` | Bitwise-OR (`|`). |
| `BIT_XOR_EXPR` | `INTEGER_TYPE` | `INTEGER_TYPE` | `INTEGER_TYPE` | Bitwise-XOR (`^`). |
| `BIT_NOT_EXPR` | ? | | `INTEGER_TYPE` | Bitwise-NOT. |
| `CALL_EXPR` | `POINTER_TYPE` to `FUNCTION_TYPE` | `TREE_LIST` of arguments | Matches function return type | Function call. |
| `CLEANUP_POINT_EXPR` | ? | ? | ? | ? |
| `COMPLEX_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | `COMPLEX_TYPE` | Construct a complex type given the real and imaginary parts. |
| `COMPONENT_REF` | `RECORD_TYPE` or `UNION_TYPE` | `FIELD_DECL` | &lt;any&gt; | Member access. |
| `COMPOUND_EXPR` | ? | ? | Same as Operand 1 | Comma operator. |
| `COMPOUND_LITERAL_EXPR` | ? | ? | ? | ? |
| `COND_EXPR` | `INTEGER_TYPE` or `BOOLEAN_TYPE` | `void` or same as result | `void` or same as Operand 1 | Either or both of operand 1 and the result must be `void` if they unconditionally `throw` or call a `noreturn` function. Operand 1 may be a `SAVE_EXPR` matching Operand 0. |
| `CONJ_EXPR` | ? | ? | ? | ? |
| `CONSTRUCTOR` | ? | ? | ? | ? |
| `CONVERT_EXPR` | &lt;any&gt; | | &lt;any&gt; | Conversion, excluding C++ superclass/subclass pointer conversions and user-defined conversions. |
| `DECL_EXPR` | | | | Local declaration; use `DECL_EXPR_DECL(decl)` to get the `*_DECL`. |
| `EQ_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | `INTEGER_TYPE` or `BOOLEAN_TYPE` | a == b (numeric only?) |
| `EXACT_DIV_EXPR` | ? | ? | ? | ? |
| `EXIT_EXPR` | condition | | | Conditionally break from the nearest enclosing `LOOP_EXPR`. |
| `FIX_TRUNC_EXPR` | `REAL_TYPE` | | `INTEGER_TYPE` or `BOOLEAN_TYPE` | Truncating conversion. |
| `FLOAT_EXPR` | `INTEGER_TYPE` or `BOOLEAN_TYPE` | | `REAL_TYPE` | Conversion. |
| `GE_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | `INTEGER_TYPE` or `BOOLEAN_TYPE` | a > b |
| `GT_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | `INTEGER_TYPE` or `BOOLEAN_TYPE` | a >= b |
| `IMAGPART_EXPR` | `COMPLEX_TYPE` | | ? | Access the imaginary part of a complex number. |
| `MODIFY_EXPR` | `VAR_DECL`, `INDIRECT_REF`, `COMPONENT_REF`, or other lvalue | &lt;any&gt; | ? | Assignment operation given LHS and RHS. |
| `INDIRECT_REF` | `POINTER_TYPE` or `REFERENCE_TYPE` | | &lt;any&gt; | Deferencing. |
| `INIT_EXPR` | `VAR_DECL`, `INDIRECT_REF`, `COMPONENT_REF`, or other lvalue | &lt;any&gt; | ? | Same as `MODIFY_EXPR` but for initialization. |
| `INTEGER_CST` | ? | ? | `INTEGER_TYPE` | Integer constant. |
| `LE_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | `INTEGER_TYPE` or `BOOLEAN_TYPE` | a < b |
| `LOOP_EXPR` | | | | Loop forever until a descendant `EXIT_EXPR` runs. |
| `LSHIFT_EXPR` | `INTEGER_TYPE` | number of bits to shift | Same as Operand 0 | Bitshift left. Value undefined if Operand 1 evaluates to a number creater than `TYPE_SIZE(TREE_TYPE(a))`. |
| `LT_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | `INTEGER_TYPE` or `BOOLEAN_TYPE` | a <= b |
| `MINUS_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | Same as Operand 0? | Subtraction (a - b). |
| `MULT_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | Same as Operand 0? | Multiplication. |
| `NE_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | `INTEGER_TYPE` or `BOOLEAN_TYPE` | a != b (numeric only?) |
| `NEGATE_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | | `INTEGER_TYPE` or `REAL_TYPE` | Unary negation. |
| `NON_LVALUE_EXPR` | &lt;any&gt; | | &lt;any&gt; | Wraps an operand to indicate that it's not an lvalue. |
| `NOP_EXPR` | &lt;any&gt; | | &lt;any&gt; | Conversion that produces no bytecode. |
| `PLUS_EXPR` | `INTEGER_TYPE` or `REAL_TYPE` | Same as Operand 0 | Same as Operand 0? | Addition. |
| `POSTDECREMENT_EXPR` | ? | ? | `INTEGER_TYPE` or `REAL_TYPE` or `BOOLEAN_TYPE` | `x--` |
| `POSTINCREMENT_EXPR` | ? | ? | `INTEGER_TYPE` or `REAL_TYPE` or `BOOLEAN_TYPE` | `x++` |
| `PREDECREMENT_EXPR` | ? | ? | `INTEGER_TYPE` or `REAL_TYPE` or `BOOLEAN_TYPE` | `--x` |
| `PREINCREMENT_EXPR` | ? | ? | `INTEGER_TYPE` or `REAL_TYPE` or `BOOLEAN_TYPE` | `++x` |
| `RDIV_EXPR` | ? | ? | ? | Floating-point division. |
| `REALPART_EXPR` | `COMPLEX_TYPE` | | ? | Access the real part of a complex number. |
| `RETURN_EXPR` | `RESULT_DECL`, `MODIFY_EXPR`, `INIT_EXPR`, or `NULL_TREE` | | | Null tree if no return value; else either containing function's result decl, or a modify/init expr acting directly on the result decl. |
| `RSHIFT_EXPR` | `INTEGER_TYPE` | number of bits to shift | Same as Operand 0 | Bitshift right. (Sign-extending shift if operand type is signed.) Value undefined if Operand 1 evaluates to a number creater than `TYPE_SIZE(TREE_TYPE(a))`. |
| `SAVE_EXPR` | &lt;any&gt; | | same as operand 0? | Memoize an expression to prevent duplicate side-effects |
| `STMT_EXPR` | | | &lt;any&gt; | See below. If no result, uses `void` type. |
| `SWITCH_EXPR` | ? | ? | ? | |
| `TARGET_EXPR` | ? | ? | ? | ? |
| `THROW_EXPR` | code | | ? | See `emit_throw`. |
| `TRUNC_DIV_EXPR` | ? | ? | `INTEGER_TYPE` | Integer division, with truncation. |
| `TRUNC_MOD_EXPR` | ? | ? | `INTEGER_TYPE` | See below. |
| `TRUTH_AND_EXPR` | ? | ? | ? | Logical-AND without short-circuiting. |
| `TRUTH_ANDIF_EXPR` | `INTEGER_TYPE` or `BOOLEAN_TYPE` | `INTEGER_TYPE` or `BOOLEAN_TYPE` | `INTEGER_TYPE` or `BOOLEAN_TYPE` | Logical-AND with short-circuiting. |
| `TRUTH_OR_EXPR` | ? | ? | ? | Logical-OR without short-circuiting. |
| `TRUTH_ORIF_EXPR` | `INTEGER_TYPE` or `BOOLEAN_TYPE` | `INTEGER_TYPE` or `BOOLEAN_TYPE` | `INTEGER_TYPE` or `BOOLEAN_TYPE` | Logical-OR with short-circuiting. |
| `TRUTH_NOT_EXPR` | `INTEGER_TYPE` or `BOOLEAN_TYPE` | | ? | Logical-NOT. |
| `TRUTH_XOR_EXPR` | ? | ? | ? | Logical-XOR without short-circuiting. |
| `VA_ARG_EXPR` | ? | ? | ? | ? |
| `VTABLE_REF` | ? | ? | ? | ? |