
# GCC functions for building value and expression nodes

## `build1`

Builds a tree node with a single `TREE_OPERAND`.

```c++
// tree.h
extern tree build1(enum tree_code code, tree type, tree operand_or_type);
```

* Not sure what it means to use a type instead of an operand. Any bullet points here that make reference to things happening "based on the operand" occur only if the argument *is* an operand.
* Defaults the result's source location to `UNKNOWN_LOCATION`.
* Sets fields on the result as appropriate for the operand:
  * `TREE_CONSTANT`
  * `TREE_READONLY`
  * `TREE_SIDE_EFFECTS`
* Handles special cases: `ADDR_EXPR`, `INDIRECT_REF`, `VA_ARG_EXPR`.


## `build2`

Builds a tree node with two `TREE_OPERAND`s.

```c++
// tree.h
extern tree build1(enum tree_code code, tree type, tree operand_a, tree operand_b);
```

* Sets fields on the result as appropriate:
  * `TREE_CONSTANT`
    * Divisions by zero are non-constant.
  * `TREE_READONLY`
  * `TREE_SIDE_EFFECTS`
  * `TREE_THIS_VOLATILE`


# GCC functions related to generating types and declarations

## Creating C bitfields

* `build_nonstandard_integer_type` for the bitfield decl's type

## General

* `build_bitint_type` for building `_BitInt`s
* `build_method_type` for building methods' function types


# General operations

* `build_empty_stmt` builds an empty statement (i.e. `;`) in the form of a `NOP_EXPR`
* `build_builtin_unreachable` builds a call to `__builtin_unreachable()`.
* `get_containing_scope` returns `TYPE_CONTEXT` or `DECL_CONTEXT` as appropriate, but I still don't know the full set of nodes that can be a "context" yet.
  * `get_ultimate_context` gets the containing `TRANSLATION_UNIT_DECL` if any of a `DECL` or `TYPE`
  * `decl_function_context` gets a `DECL` or `TYPE`'s innermost enclosing function
  * `decl_type_context` gets a `DECL` or `TYPE`'s innermost enclosing type
