
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

* `list_node` is a wrapper around tree lists &mdash; not to be confused with tree chains. "Lists" in this context are key/value maps wherein each list node has a key (`TREE_PURPOSE(node)`) and a value (`TREE_VALUE(node)`).
* `statement_list` is a wrapper around statement lists, with helper functions for appending expressions and other lists.
* `value` is an abstraction for any node that can be used in expressions. This can include expressions themselves, or it can include certain kinds of declarations, such as variable and function-parameter declarations.
* `decl::base` represents any `*_DECL` node. Subclasses exist for some declaration types. Declarations that can be used as values offer an `as_value()` member function which converts them to a `value`.
* `expr::base` represents any `*_EXPR` node. Subclasses exist for some expression types. All expressions are considered usable as values, so `expr::base` subclasses `value`.
* `type::base` represents any `*_TYPE` node. Subclasses exist for specific varieties.
* Structures in the `flow` namespace exist as helpers to generate things like `for` loops.

The wrappers for types, values, declarations, and expressions should be treated similarly to pointers: you can use them to wrap arbitrary nodes (e.g. `decl::base::from_untyped(node)`), and they will assert that the node is either `NULL_TREE` or a node of the correct `TREE_CODE`. You can check if a wrapper is empty by calling `empty()` on it.


## Defects

* The wrapper classes act as views, but some have constructors which actually create a new tree node for you. The issue is that some wrappers' default constructors do so (e.g. `expr::local_block`), while most wrappers' default constructors do not (creating instead a wrapper around `NULL_TREE`). This is generally only done when it is convenient, but it's still inconsistent, and I should revise the API to be more explicit about when nodes are created.
* `gw::value` should use the GCC function `build_binary_op` to generate all binary operators that `build_binary_op` supports. That function handles integer promotions, type validation, C pointer arithmetic, and so on. We currently avoid it in most places because it can emit user-visible warnings and errors (I would prefer assertions since the use case here is code generation under the hood).
* Similarly, `gw::value` should use `build_unary_op` for more unary operations. I currently only use it where I know it to be needed for correctness deep inside the compiler (e.g. `ADDR_EXPR`, to properly update things like `TREE_ADDRESSABLE` on the operands).
  * Not all unary operations are safe to use with `build_unary_op`; for example, `INDIRECT_REF`, the pointer-dereferencing operator, isn't compatible and will cause assertion failures or crashes if built via `build_unary_op`.


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
* `CONSTRUCTOR`
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