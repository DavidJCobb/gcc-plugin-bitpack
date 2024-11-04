
# `list_node`

A simple wrapper for any nodes that form a key/value map. This wrapper offers an iterator and some accessors. Under the hood, the map is stored as a linked list of `TREE_LIST` nodes, where each node is a key/value pair: use `TREE_PURPOSE(node)` to access the key, `TREE_VALUE(node)` to access the value, and `TREE_CHAIN(node)` to access the next pair.

This should not be confused with `chain_node`, which wraps nodes that form a linked list via `TREE_CHAIN`. Use `chain_node` when the chained tree nodes are meaningful in and of themselves.