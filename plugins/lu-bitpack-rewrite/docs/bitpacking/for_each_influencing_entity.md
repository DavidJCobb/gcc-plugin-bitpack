
# `bitpacking::for_each_influencing_entity`

## `gw::type::base` overload

Given a type, execute a functor for each influencing entity, including the type itself and including the `typedef` that declared it (if any), in order from lowest to highest priority.

Influence is defined as follows:

* An array type (higher priority) is influenced by its value type (lower priority).
* A `typedef` (higher priority) is influenced by the type it defines (middle priority), and by the type it is based on (lower priority).
  * A note on arrays: for `typedef int a [3]` which declares `a` as equivalent to an array of 3 `int`s such that `std::is_same_v<a, int[3]>`, "the type it is based on" is `int[3]`, not `int`.

Typedefs are included in iteration because attributes applied to a typedef may end up on the `TYPE_DECL` rather than the created `TYPE`.


### Examples

These examples assume we run on the type named by identifier `x`.

#### Example A

```c
typedef int a;
typedef a   x;
```

We execute on:

| `TREE_CODE` | Name | Described entity |
| :- | :- | :- |
| `INTEGER_TYPE` | `int` | `int` |
| `TYPE_DECL` | `int` | Built-in `typedef` declaring `int`. |
| `INTEGER_TYPE` | `a` | The new type created by the `typedef` for `a`. This type is a synonym for `int`. |
| `TYPE_DECL` | `a` | The `typedef` for `a`. |
| `INTEGER_TYPE` | `x` | The new type created by the `typedef` for `x`. This type is a synonym for `x`. |
| `TYPE_DECL` | `x` | The `typedef` for `x`. |

#### Example B

```c
typedef int x [3]; // x is an array of 3 ints
```

We execute on:

| `TREE_CODE` | Name | Described entity |
| :- | :- | :- |
| `INTEGER_TYPE` | `int` | `int` |
| `TYPE_DECL` | `int` | Built-in `typedef` declaring `int`. |
| `ARRAY_TYPE` | <unnamed> | `std::is_same_v<__unnamed_x_underlying_type, int[3]>` |
| `ARRAY_TYPE` | `x` | The new type created by the `typedef` for `x`. This type is a synonym for `__unnamed_x_underlying_type`. |
| `TYPE_DECL` | `x` | The `typedef` for `x`. |

#### Example C

```c
typedef int a;
typedef a   b [3]; // b is an array of 3 a
typedef b   x;
```

We execute on:

| `TREE_CODE` | Name | Described entity |
| :- | :- | :- |
| `INTEGER_TYPE` | `int` | `int` |
| `TYPE_DECL` | `int` | Built-in `typedef` declaring `int`. |
| `INTEGER_TYPE` | `a` | The new type created by the `typedef` for `a`. This type is a synonym for `int`. |
| `TYPE_DECL` | `a` | The `typedef` for `a`. |
| `ARRAY_TYPE` | <unnamed> | `std::is_same_v<__unnamed_c_underlying_type, a[3]>` |
| `ARRAY_TYPE` | `b` | The new type created by the `typedef` for `b`. This type is a synonym for `__unnamed_b_underlying_type`. |
| `TYPE_DECL` | `b` | The `typedef` for `b`. |
| `ARRAY_TYPE` | `x` | The new type created by the `typedef` for `x`. This type is a synonym for `b`. |
| `TYPE_DECL` | `x` | The `typedef` for `b`. |

#### Example D

```c
typedef int a;
typedef a   b;
typedef b   c [3]; // c is an array of 3 b
typedef c   x [5]; // x is an array of 5 c
```

We execute on:

| `TREE_CODE` | Name | Described entity |
| :- | :- | :- |
| `INTEGER_TYPE` | `int` | `int` |
| `TYPE_DECL` | `int` | Built-in `typedef` declaring `int`. |
| `INTEGER_TYPE` | `a` | The new type created by the `typedef` for `a`. This type is a synonym for `int`. |
| `TYPE_DECL` | `a` | The `typedef` for `a`. |
| `INTEGER_TYPE` | `b` | The new type created by the `typedef` for `b`. This type is a synonym for `a`. |
| `TYPE_DECL` | `b` | The `typedef` for `b`. |
| `ARRAY_TYPE` | <unnamed> | `std::is_same_v<__unnamed_c_underlying_type, b[3]>` |
| `ARRAY_TYPE` | `c` | The new type created by the `typedef` for `c`. This type is a synonym for `__unnamed_c_underlying_type`. |
| `TYPE_DECL` | `c` | The `typedef` for `c`. |
| `ARRAY_TYPE` | <unnamed> | `std::is_same_v<__unnamed_x_underlying_type, c[5]>` |
| `ARRAY_TYPE` | `x` | The new type created by the `typedef` for `x`. This type is a synonym for `__unnamed_x_underlying_type`. |
| `TYPE_DECL` | `x` | The `typedef` for `x`. |