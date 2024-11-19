
# `gcc_wrappers`

An attempt at creating a more type-safe interface to GCC's untyped `tree` nodes.

Goals:

* Stricter typing
* Add convenient, relevant accessors, so I don't constantly have to rummage around GCC internals
  * Many, many identifiers are declared in `foo.h` only to be defined in some completely differently-named `bar/baz.cc`. Most search engines I've been using -- even those specifically built for code -- miss identifiers *constantly*, so this is quite painful.
* Improved ergonomics
* Improved version independence via built-in version checks wherever I am aware of the need for them
* Abstracted access to important GCC functions that are exported but not included in the plug-in headers (e.g. stuff from `c/c-tree.h` like the C-language `comptypes`, where plug-ins are given the C/C++ and the C++-exclusive headers but not the C-exclusive headers)

An example of how these wrappers look when generating a function declaration and definition from scratch:

```c++
namespace gw {
   using namespace gcc_wrappers;
}

// Generate `void __generated_function_T(T* __arg_0)` for some type T:

gw::decl::function make_function(gw::type type) {
   auto int_type  = gw::builtin_types::get().basic_int;
   auto void_type = gw::builtin_types::get().basic_void;

   gw::decl::function result;
   {
      auto func_name = std::string("__generated_function_" + type.name());
      auto func_type = gw::type::make_function_type(
         // return type:
         void_type,
         // argument types:
         type.add_pointer()
      );
      result = gw::decl::function(func_name, func_type);
   }
   auto dst = result.as_modifiable();
   dst.set_result_decl(gw::decl::result(void_type));
   
   gw::expr::local_block root_block;
   auto statements = root_block.statements();
   
   // Assume that the struct has a member `int foo[10]`.
   // Let's create a loop to set all of those integers 
   // to `i + 3`.
   //
   {
      //
      // First, we create the loop, specifying the value 
      // type of its counter variable. Then, we can set 
      // the counter bounds.
      //
      gw::flow::simple_for_loop loop(int_type);
      loop.counter_bounds = {
         .start     = 0,
         .last      = 9,
         .increment = 1,
      };
      
      //
      // At this point, the loop has created declarations 
      // for its counter variable and internal labels. You 
      // can use the counter variables in expressions, and 
      // you can generate `gw::expr::go_to_label`s if you 
      // need to `break` or `continue`.
      //
      
      gw::statement_list loop_body;
      
      loop_body.append(
         //
         // `__arg_0->foo[i] = i + 3`, expressed by way of 
         // the form `(*__arg_0).foo[i] = i + 3`:
         //
         gw::expr::assign(
            result.nth_argument(0).as_value().deference()
               .access_member("foo").access_array_element(loop.counter.as_value()),
            //
            // =
            //
            loop.counter.as_value().add(
               gw::expr::integer_constant(int_type, 3)
            )
         )
      );
      
      //
      // We want to tell the loop to generate its local 
      // block and relevant expressions, including those 
      // expressions to actually place its labels somewhere.
      //
      loop.bake();
      
      statements.append(loop.enclosing);
   }
   
   //
   // We've generated the code we want to, so let's now go 
   // ahead and finalize our function.
   //
   // We want this function to make it into the object file, 
   // so let's mark it as non-extern before we commit its 
   // root block.
   //
   dst.set_is_defined_elsewhere(false);
   dst.set_root_block(root_block);
   
   //
   // As a bonus, let's also bind the function to the current 
   // scope, so that name lookups (`lookup_name`) can find it.
   //
   // The "current scope" is, AFAIK, whatever scope GCC is 
   // parsing at the time this code runs. I don't know of a more 
   // direct or controlled way to bind declarations to, say, the 
   // file scope specifically.
   //
   dst.introduce_to_current_scope();

   return result;
}
```

## Classes offered

* `chain_node` is a wrapper around tree chains, which act as linked lists.
* `list_node` is a wrapper around tree lists, which, despite the name, act as key/value maps. (They are linked lists of "pair" nodes.)
* `statement_list` is a wrapper around statement lists, with helper functions for appending expressions and other lists.
* `value` is an abstraction for any node that can be used in expressions. This can include expressions themselves, or it can include certain kinds of declarations, such as variable and function-parameter declarations.
* `decl::base` represents any `*_DECL` node. Subclasses exist for some declaration types. Declarations that can be used as values offer an `as_value()` member function which converts them to a `value`.
* `expr::base` represents any `*_EXPR` node. Subclasses exist for some expression types. All expressions are considered usable as values, so `expr::base` subclasses `value`.
* `type::base` represents any `*_TYPE` node. Subclasses exist for specific varieties.
* Structures in the `flow` namespace exist as helpers to generate things like `for` loops. They are not wrappers in themselves, but do use the wrappers.

The wrappers for types, values, declarations, and expressions should be treated similarly to pointers: you can use them to wrap arbitrary nodes (e.g. `decl::base::from_untyped(node)`), and they will assert that the node is either `NULL_TREE` or a node of the correct `TREE_CODE`. You can check if a wrapper is empty by calling `empty()` on it.


## Lifetime

My research here is incomplete, and the wrappers offer no facilities for dealing with this directly.

GCC uses garbage collection internally ("GGC"). The allocator keeps track of every tree node and periodically, GCC will start at all top-level objects and traverse over them to check which tree nodes are reachable. Unreachable nodes are deleted. Garbage collection doesn't run spontaneously, but rather must be triggered explicitly via calls to `ggc_collect`. This means that you don't need to explicitly delete tree nodes that you create. Some operations may create and discard nodes (e.g. starting at a type node for `T` and getting a type node for `const T*` via `type.add_const().add_pointer()`); this is fine, because the intermediate nodes can be garbage-collected later.

[The `_GTY()` macro](https://gcc.gnu.org/onlinedocs/gccint/Type-Information.html) is used to annotate top-level objects (the [roots](https://gcc.gnu.org/onlinedocs/gccint/GGC-Roots.html)). The macro acts as a signal to `gengtype`, a GCC code generation tool that understands a limited subset of C++.

If you have singletons or globals that may hold on to GGC-controlled objects, then you'll need to mark them with `_GTY()` and [run `gengtype` in plug-in mode](https://gcc.gnu.org/onlinedocs/gccint/Files.html) to generate garbage-collection code.[^plugin-gengtype-when] Because `gengtype` only supports a very limited subset of C++ syntax, you may need to [hand-write your own code](https://gcc.gnu.org/onlinedocs/gccint/User-GC.html) to help the garbage collector traverse your objects.

[^plugin-gengtype-when]: GCC seems to have had multiple plug-in APIs over the years. The mention of `gengtype` having a "plugin mode" [dates back to 2009](https://github.com/gcc-mirror/gcc/commit/bd117bb6b44870ca006eb12630a454302873674e) (circa GCC 4). The current plug-in API dates back to around 2009 as well, so safe to say that the `gengtype` integration described would apply to the current plug-in API.

### Garbage-collected GCC objects

If your singleton stores these objects, then it'll need to pass them to GGC functions in handwritten overloads for `gt_ggc_mx` and friends.

This list is non-exhaustive.

* `tree`
  * NOTE: Given a `location_t loc`, `LOCATION_BLOCK(loc)` is a `tree`. It seems like GCC prefers to capture it in a local and pass the local to `gt_ggc_mx` and friends, rather than passing the macro expression directly.
* `gimple_seq`
* `rtx`


### An overview of GCC's memory management

As of this writing, GGC and `gengtype` are, uh, *not terribly well-explained*, so here's my try at actually communicating how they work.

GGC is a mark-and-sweep garbage collector. It starts at "root" objects and traverses down through them to mark all seen garbage-collectible objects, using `gengtype`-generated code to navigate the structures. Any objects that aren't marked by the end of a pass are assumed to be unreachable and are disposed of.

Objects are garbage-collectible if they're managed using any of these functions:

* `ggc_alloc`
* `ggc_vec_alloc`
* `ggc_cleared_alloc`
* `ggc_cleared_vec_alloc`
* `ggc_alloc_atomic`
* `ggc_alloc_no_dtor`
* `ggc_alloc_string`
* `ggc_strdup`
* `ggc_realloc`
* `ggc_delete`
* `ggc_free`

While reading about `gengtype`, you'll see references to "PCH," which stands for "pre-compiled header." The particular way PCHs are talked about in the documentation won't make sense unless you know that [GCC implements pre-compiled headers as snapshots of the entire heap](https://stackoverflow.com/a/12438040)[^pch-savestate], akin to savestates in a game console emulator or snapshots in a VM. Whenever you see "a PCH" in documentation related to GGC or `gengtype`, mentally substitute the phrase out for "a savestate."

[^pch-savestate]: PCH savestates are created with `gt_pch_save` and loaded with `gt_pch_restore`.

If you have a data structure that needs to act as a GC root, e.g. a singleton that refers (directly or indirectly) to garbage-collectible objects, then you'll need to annotate it for `gengtype`. If your singleton is too complex for `gengtype` to understand, then you can annotate it with the `user` option, to tell `gengtype` that you'll be supplying user-defined functions to mark the garbage-collectible objects that your singleton refers to.

```c++
extern GTY((user)) my_struct_type my_struct_instance;
```

The three functions you'll need to define are:

* `void gt_ggc_mx(my_struct* instance);`
  * This function marks the garbage-collectible objects that `instance` refers to. All you have to do is recursively call it (i.e. `gt_ggc_mx(instance->field_to_mark)`) on any fields that are pointers to collectible objects.
* `void gt_pch_nx(my_struct* instance);`
  * This function is basically the same thing as `gt_ggc_mx`, but for operations related to a PCH (i.e. a savestate). Recursively call `gt_pch_nx` on the pointers to collectible objects.
* `void gt_pch_nx(my_struct* instance, gt_pointer_operator op, void* cookie);`
  * This function is used for operations related to a PCH (i.e. a savestate). It may passively read your struct instance (e.g. to write a new savestate to disk), or it may actively modify your struct instance (e.g. to load a savestate into memory, performing pointer fix-ups as necessary).
  * This function needs to call the given pointer operator on every garbage-collectible field. That is: given some garbage-collectible type `T`, if your object contains some `T* field`, you need to call `op(&field, nullptr, cookie)` such that the operator receives a `T**`.

Moving from prose to code, the functions look like this:

```c++
void gt_ggc_mx(my_struct* instance) {
  gt_ggc_mx(instance->field_to_mark);
}

void gt_pch_nx(my_struct* instance) {
  gt_pch_nx(instance->field_to_mark);
}

void gt_pch_nx(my_struct* instance, gt_pointer_operator op, void* cookie) {
  op(&(instance->field_to_mark), NULL, cookie);
}
```

You'll note that you can only mark garbage-collectible fields, or forward an operation to run on garbage-collectible fields; there doesn't appear to be any way to store or restore your object's state here. This can be seen on GCC internals as well. For example, their `vec` template is `GTY((user))` since it's a template, and the only thing its GGC functions do is recurse on the vector elements; nothing is done to store the vector's length. It's obvious, then, how GGC marks collectible objects, and how it does pointer fix-ups; but it's not clear how it actually stores object data. Perhaps savestates can only record the contents of collectible objects, such that a singleton would be wiped clean unless it's under GGC's control? (But `vec` doesn't *seem* to be allocated through `ggc_alloc` and friends? And don't some `tree` structures store `vec`s?)

There also exists a `ggc_register_root_tab` function that is noted as being "useful for some plugins." No clue when or how to use it.


## Defects

* `gw::value` should use the GCC function `build_binary_op` to generate all binary operators that `build_binary_op` supports. That function handles integer promotions, type validation, C pointer arithmetic, and so on. We currently avoid it in most places because it can emit user-visible warnings and errors (I would prefer assertions since the use case here is code generation under the hood).
* Similarly, `gw::value` should use `build_unary_op` for more unary operations. I currently only use it where I know it to be needed for correctness deep inside the compiler (e.g. `ADDR_EXPR`, to properly update things like `TREE_ADDRESSABLE` on the operands).
  * Not all unary operations are safe to use with `build_unary_op`; for example, `INDIRECT_REF`, the pointer-dereferencing operator, isn't compatible and will cause assertion failures or crashes if spawned via `build_unary_op`.
* It would probably be more appropriate to have `gw::constant::base` as the base class for `gw::expr::integer_constant` and `gw::expr::string_constant`, with the base class for constants subclassing `gw::value` as `gw::expr::base` currently does.
* The codebase isn't consistently const-correct.
* Const-correctness doesn't work super well since these are structs rather than pointers or references: even if you have a `const gw::type::base`, it's trivial to un-const it by just assigning it to a non-const `gw::type::base`. The same thing applies to when a const wrapper tries to return another const wrapper.

### Failure to distinguish between "pointerness" and "referenceness"

In C++, you have three basic kinds of types (related to this defect, anyway):

* Values
* References
* Pointers

In this context, a <dfn>value</dfn> is an object that you have *right here, right now*, in your space and under your ownership. A <dfn>reference</dfn> is the means by which you access a value that definitely exists, but somewhere else, potentially outside of your control. A <dfn>pointer</dfn> is the means by which you access a value that *may or may not* exist somewhere else.

The problem with these wrappers is that they elide the distinctions between these three things. In the first place, we lose the ability to express "pointerness" versus "referenceness:" all wrappers *may* wrap a node of a constrained set of tree codes, or they may wrap `NULL_TREE`. In the second place, some wrappers auto-create nodes when instantiated (e.g. `expr::local_block`), while most don't (instead spawning an empty wrapper).

#### Potential solution: wrappers for "references;" separate "pointer" classes

One potential way to solve this would be bifurcated types. We could have all of the individual wrapper types that we currently do, each of which would hold a `tree` internally, but not allow them to be `empty()`; and then we could define `template<typename Wrapper> class _optional_wrapper` which can be empty. Invoking `_optional_wrapper<T>::operator*` or `_optional_wrapper<T>::operator->` would basically just cast it to the wrapped type.

So:

```c++
namespace gcc_wrappers {
   template<typename Wrapper>
   class _optional_wrapper {
      protected:
         tree _node = NULL_TREE;
   
      public:
         Wrapper operator* noexcept {
            assert(this->_node != NULL_TREE);
            return Wrapper::wrap(this->_node); // `from_untyped` in current codebase
         }
   };
   
   namespace expr {
      class base /* ... */ {
         public:
            /* ... */
      };
      
      using base_ptr = _optional_wrapper<base>;
   }
}

void example() {
   base_ptr foo;
   
   static_assert(std::is_same_v<
      decltype(*foo),
      base
   >);
}
```

We'd additionally require that `T::wrap`, the replacement for `T::from_untyped`, assert that the passed-in tree node is non-null; and we'd make it so that non-pointer wrappers create tree nodes when constructed through any means but `T::wrap` (and if we haven't implemented creating a given tree node type, then we'll simply expose no public constructors, such that the only way to get a `T` wrapper is by dereferencing `T_ptr`).

This still isn't perfect, though, because the behavior of copy- and move-assignment is undefined. A `T` acts like a value when constructed, but should it act like a value when copy- or move-assigned? If so, then we lose the ability to express "referenceness," choosing only to represent values and pointers. If not, then either: we have something that behaves like a value when constructed through anything other than copy-construction or move-construction, yet behaves like a reference otherwise; or we disable conventional construction for consistency's sake, only allowing construction via static member functions or non-member functions, which feels less ergonomic.

A third option is to *tri*furcate the types: `T`, `T_ptr`, and `T_ref`. Each of these classes would wrap a `tree` and just implement different semantics with respect to construction, assignment, and so on. We could have `T_ref` derive from `T` and override construction and assignment in order to allow the same (wrapper-specific) member functions to be invoked on both, while having `T_ptr` produce a `T_ref` when dereferenced.

In deciding between these approaches (bifurcate with inconsistency; bifurcate with verbosity; trifurcate) it is worth considering that there aren't *terribly* many nodes that are safe to directly modify. For example, I wouldn't recommend altering `TREE_READONLY` on `int_type_node`. I've endeavored to make it so that `gcc_wrappers::type::base` is incapable of directly modifying a type, instead returning modified copies, and in that case there's little practical difference between `T` and `T_ref` beyond the former creating a new type (which is then impossible to configure) when instantiated.

Something else worth considering is that if we imitate references as closely as possible, then `T_ref` wouldn't be directly constructible; you'd only be able to construct it via `T_ref::wrap` or by dereferencing a `T_ptr`. (If you could default-construct it, then it could be empty.) This means that non-default constructors-with-args wouldn't be ambiguous: it's not meaningful to "construct" a reference, least of all from anything other than a referent, so clearly what you're constructing in that case is a value (i.e. a new tree node). But then the meaning of copy-construction becomes ambiguous (are we creating a new reference to the same referent, or are we creating a new referent?).

Hm. I dislike boilerplate, but perhaps the best option would nonetheless be to have `T` mimicking reference semantics, `T_ptr` or `T_opt` mimicking pointer semantics, and `static T T::make()` or `T node = new_node{}`[^new-node] as the means to actually spawn a new node (preferring the static member function over anything that looks like "magic").

[^new-node]: The `new_node` type would be an empty tag/sentinel type, wherein if we actually know how to spawn a given tree node, then we give its wrapper an implicit-conversion constructor from `new_node` so that this assignment works. We can't prevent `auto foo = new_node{}` from working, but that'd lead to type errors when you actually try to do anything with `foo` anyway.

## Potential future plans

* More `flow` wrappers to simplify common control flow structures
  * `while`
  * `do` ... `while`
  * `switch`/`case`, including with fall-through
    * We need not support things like Duff's device here.
* Consider renaming `decl::base::is_defined_elsewhere` and friends to `decl::base::is_extern`. Some GCC comments seem to suggest that `DECL_EXTERN` doesn't correspond 1:1 with the C/C++ `extern` keyword, else I'd have used that name to start with.

### Missing features

* `ARRAY_RANGE_REF`. There's a member function declaration on `value` but no implementation.

### To-be-implemented expression types

Potentially notable ones:

* `EXIT_EXPR`: Conditionally breaks out of the nearest containing `LOOP_EXPR`. The sole operand is the condition to test, and we exit if the result is nonzero.
  * `EXIT_EXPR` should only appear somewhere inside of a `LOOP_EXPR`.
* `LOOP_EXPR`: An infinite loop. `LOOP_EXPR_BODY(expr)` is the loop body. The loop executes repeatedly until an `EXIT_EXPR` is run.
  * Based on a look at GCC source, apparently only produced by C++ and Fortran?
* `INIT_EXPR`: Same as `MODIFY_EXPR`, but used for when struct variables' members are initialized.
  * This appears to be used in C++ for aggregate initialization and for initialization of struct members generally. Based on a look at the latest source for GCC, it's never produced by the C parser.
* `SAVE_EXPR`: Wraps an expression (the first operand) and prevents the expression from being executed more than once (e.g. memoization). The purpose is to avoid repetition of an expression's side-effects where the expression's result should be used more than once.
* `STMT_EXPR`: GCC extension for treating statements as expressions e.g. `return ({ int j = 3; j + 1; });`.
  * `STMT_EXPR_STMT(expr)` is the statement contained in the expression, and is always a `COMPOUND_STMT`.
  * If the last direct child of `STMT_EXPR_STMT(expr)` is an `EXPR_STMT`, then the value it computes is the result of the `STMT_EXPR`. Otherwise, the `STMT_EXPR` yields no value and its type is `void`.

Unusual ones (see [docs](https://gcc.gnu.org/onlinedocs/gcc-3.4.3/gccint/Expression-trees.html#Expression-trees)):

* `AGGR_INIT_EXPR`
* `CLEANUP_POINT_EXPR`
* `CONSTRUCTOR`: Used for braced initializers for arrays, structs, and unions.
  * `CONSTRUCTOR_ELTS(node)` is a `vec<constructor_elt, va_gc>*` instance. Any fields or elemnets not initialized are cleared unless the `CONSTRUCTOR_NO_CLEARING` flag is set.
  * `FOR_EACH_CONSTRUCTOR_VALUE (vals, i, val) { ... }` exists for iterating over constructor elements.
  * If the initializer is not for an array, then each `constructor_elt`'s `index` is a `FIELD_DECL`.
  * If the initializer is for an array, then each `constructr_elt`'s `index` is either an `INTEGER_CST` or a `RANGE_EXPR`. The latter case occurs as a shorthand when several array indices are initialized to the same value (with any side effects being recomputed per value unless the value is a `SAVE_EXPR`).
  * `build_constructor` (`tree.h`) can create these nodes given an existing `vec` of constructor elements. `build_constructor_single` exists for when you just need one index node and one value node, and `build_constructor_from_list` exists for when the indices and values exist in a tree list.
  * `recompute_constructor_flags(node)` can update a `CONSTRUCTOR` node's internal flags if its contents change. `verify_constructor_flags(node)` can assert the current flags' correctness.
  * Append a new constructor element via `CONSTRUCTOR_APPEND_ELT(node, index_node, value_node)`.
* `COMPOUND_LITERAL_EXPR`
* `TARGET_EXPR`
* `VA_ARG_EXPR`
* `VTABLE_REF`
* `NON_LVALUE_EXPR`: wraps a single operand to indicate that it's not an lvalue. Often, you can just unwrap it and deal with the operand directly.

Conversion and decomposition operators:

* `COMPLEX_EXPR`: construction of a `COMPLEX_TYPE`-type value given two operands: the real and imaginary parts. Both operands are of the same type, and can be integers or reals.
* `IMAGPART_EXPR` and `REALPART_EXPR`: Access the real or imaginary halves of a complex number (the sole operand).

Logical operators:

* `THROW_EXPR`: a `throw` expression. The sole operand is all code needed to perform the operation *except* for the actual call to the `__throw` function. Generated by the compiler-internal function `emit_throw`.
* `COMPOUND_EXPR`: comma operator. First operand is computed and discarded; second operand is the result.