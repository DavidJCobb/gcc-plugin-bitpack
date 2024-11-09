
# `builder`

Helper class for concatenating several strings, wherein we collect a list of strings to concatenate but only actually concatenate them at the end. This reduces the number of potentially large memory allocations, and also avoids having to repeatedly move string data when prepending multiple strings.

To-be-concatenated strings are stored as either `std::string` or `std::string_view`, depending on the type with which they're passed in. We avoid extra memory allocations in the single-string case (i.e. if you *may* concatenate strings, conditionally) by using a `std::variant` and lazy-creating a full list when a second to-be-concatenated string is added.