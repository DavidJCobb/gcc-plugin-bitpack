
# Plugin callbacks

## `PLUGIN_FINISH_DECL`

Called when a `DECL` finishes parsing. The event data is a `DECL` node (cast from `void*` to `tree` to access it).

* **GCC 11.4.0:** This runs after attributes on a `DECL` are parsed, but before attribute handlers are invoked and `DECL_ATTRIBUTES` are set.\
* **GCC 11.4.0:** When dealing with a C bitfield `FIELD_DECL`, this runs after `DECL_NONADDRESSABLE_P` is set, but before any bit-field properties are set.
  * If you need to deal with bitfield properties, then watch for `PLUGIN_FINISH_TYPE` on the field's containing type instead.

## `PLUGIN_FINISH_TYPE`

Called when a `TYPE` finishes parsing.

* **GCC 11.4.0:** Unlike `PLUGIN_FINISH_DECL`, attributes on a type tag (i.e. `struct [attrs] [name] { ... }`) *are* present here.
* By the time this runs, it is no longer possible to add attributes to the type.
* **GCC 11.4.0:** In C, this callback runs whenever a type is forward-declared. This includes any valid reference to a type that wasn't named via `typedef`, even if the type is already complete and this callback has already run on it. (That is: you know how you always have to write `struct Typename` and not just `Typename`, if it wasn't named using `typedef`? *You writing that* will cause `PLUGIN_FINISH_TYPE` to fire again.)