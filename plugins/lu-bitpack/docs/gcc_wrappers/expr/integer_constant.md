
# `integer_constant`

The wrapper for `INTEGER_CST` tree nodes.

The value of an `INTEGER_CST` node is `((TREE_INT_CST_HIGH(node) << HOST_BITS_PER_WIDE_INT) + TREE_INST_CST_LOW(node))`. The value may be wider than whatever a "wide int" is on the platform the compiler and plug-in are compiled on.

`HOST_BITS_PER_WIDE_INT` is always at least 32.