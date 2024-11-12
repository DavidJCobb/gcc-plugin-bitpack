
# `string_constant`

The wrapper for `STRING_CST` tree nodes.


## Lifetime and sharing

Unlike `INTEGER_CST`, GCC doesn't pool and reuse `STRING_CST` nodes. Every time you create such a node, you are creating an entirely new string.