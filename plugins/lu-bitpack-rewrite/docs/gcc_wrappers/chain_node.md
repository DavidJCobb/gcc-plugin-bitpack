
# `chain_node`

A simple wrapper for any nodes that form a linked list via `TREE_CHAIN`, e.g. the `*_DECL` nodes that make up a `RECORD_TYPE`'s member declarations. This wrapper offers an iterator and some accessors.

This should not be confused with `list_node`, which wraps key/value maps. There, `TREE_LIST` nodes represent a linked list of key/value pairs, where `TREE_PURPOSE(node)` is the key, `TREE_VALUE(node)` is the value, and `TREE_CHAIN(node)` is the next pair.