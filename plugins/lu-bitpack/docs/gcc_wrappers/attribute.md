
# Attributes

GCC stores attributes as a key/value map. We could use `list_node` to refer to them, but it's friendlier to define dedicated wrappers for them.

Within the map, keys are the attribute name (as an `IDENTIFIER_NODE`) and values are the attribute's arguments, if any. The arguments are themselves encoded as a key/value map, even though only the values are documnented as being meaningful.


## Implementation notes

`attribute_list` is a wrapper for a key/value pair node in the attribute list. Key/value maps are stored as linked lists of pair nodes, so the head node represents the entire map.

`attribute` also wraps a key/value node. This is both so that we can retrieve the attribute's name (since that's the key), and so we can tell "no attribute" apart from "attribute with no value," since an attribute may not have any arguments.
