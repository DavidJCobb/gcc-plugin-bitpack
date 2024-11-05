
# `codegen::serialization_value_path`

A string which represents the path to any element that was serialized directly into a sector. The string *mostly* uses C-style syntax e.g. `foo.bar[4]`. The exception is for when generated code loops over an array; to denote this, we use array slice notation, e.g. `foo.bar[2:6]` to denote iteration over the range [2, 6).

These are used in our generated XML output.