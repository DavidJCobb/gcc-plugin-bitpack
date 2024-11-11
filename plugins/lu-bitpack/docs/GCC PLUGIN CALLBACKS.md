
# Plugin callbacks

## `PLUGIN_FINISH_DECL`

Called when a `DECL` finishes parsing. The event data is a `DECL` node (cast from `void*` to `tree` to access it).

* **GCC 11.4.0:** This runs after attributes on a `DECL` are parsed, but before attribute handlers are invoked and `DECL_ATTRIBUTES` are set.
* **GCC 11.4.0:** When dealing with a C bitfield `FIELD_DECL`, this runs after `DECL_NONADDRESSABLE_P` is set, but before any bit-field properties are set.
  * If you need to deal with bitfield properties, then watch for `PLUGIN_FINISH_TYPE` on the field's containing type instead.

## `PLUGIN_FINISH_TYPE`

Called when a `TYPE` finishes parsing.