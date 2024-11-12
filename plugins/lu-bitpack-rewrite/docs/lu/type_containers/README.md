
# Type containers

**BLUF:** This folder is for arrays and similar containers built to operate at compile-time on types rather than values.

In conventional programming, the fundamental unit of computation is the *value*: you generally write instructions that create, transform, and destroy values when your program runs. Types and classes exist to define and constrain the forms that a value may take and the operations that may be performed upon the value. Functions pass values (or pointers or references to them) around as arguments, and create and return values during their operations. 

In the most basic uses of C++ templates, you write fill-in-the-blank functions and types, where those blanks can be filled in with types or with compile-time values. These functions still consist of instructions to create, transform, and destroy values when your program runs: values are still the fundamental unit of computation.

In more advanced C++ template programming, however, you'll often treat types as a parallel-universe counterpart to immutable values: you'll write operations which take a type as input and return a transformed version of that type as output. The fundamental units of computation in this compile-time "parallel universe" are values *and* types. It's useful, then, to have containers that operate on types: arrays of types; type/type maps, type/value maps, and value/type maps; and so on.